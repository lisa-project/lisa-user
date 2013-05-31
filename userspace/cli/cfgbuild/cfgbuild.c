#include "swcli.h"
#include <sys/stat.h>
#include <sys/types.h>

/* max len is for ",nnnn-nnnn" */
#define MAX_VLAN_TOKEN 11

static __inline__ int list_vlans_token(unsigned char *bmp, int vlan, char *token)
{
	int i, min;
	while (vlan <= SW_MAX_VLAN && sw_forbidden_vlan(bmp, vlan))
		vlan++;
	if (vlan > SW_MAX_VLAN)
		return 0;
	i = snprintf(token, MAX_VLAN_TOKEN, "%d", vlan);
	assert(i < MAX_VLAN_TOKEN);
	min = vlan;
	while (vlan <= SW_MAX_VLAN && sw_allowed_vlan(bmp, vlan))
		vlan++;
	if (vlan - min == 1)
		return vlan;
	if (vlan - min > 2) {
		i = snprintf(token + i, MAX_VLAN_TOKEN, "-%d", vlan - 1);
		assert(i < MAX_VLAN_TOKEN);
	} else
		vlan--;
	return vlan;
}

static __inline__ void list_vlans(FILE *out, unsigned char *bmp)
{
	char buf[80];
	char token[MAX_VLAN_TOKEN];
	int vlan = 1;
	int first = 1;
	int i;

	token[0] = ',';
	i = sprintf(buf, " switchport trunk allowed vlan ");
	while ((vlan = list_vlans_token(bmp, vlan, token + 1))) {
		if (i + strlen(token + first) >= sizeof(buf)) {
			fputs(buf, out);
			fputc('\n', out);
			i = sprintf(buf, " switchport trunk allowed vlan add ");
			first = 1;
		}
		i += sprintf(&buf[i], &token[first]);
		first = 0;
	}
	fputs(buf, out);
	fputc('\n', out);
}

#define nbit2mask(nbit)	(htonl(nbit ? (~((uint32_t)0)) ^ ((((uint32_t)1) << (32 - nbit)) - 1) : 0))
int build_config_interface(struct cli_context *ctx, FILE *out, struct net_switch_device *nsdev, int if_cmd)
{
	int status;
	unsigned char bmp[SW_VLAN_BMP_NO];
	char desc[SW_MAX_PORT_DESC];
	int need_trunk_vlans = 1;
	struct cdp_session *cdp;
	int ret = CLI_EX_OK;
	int flags, access_vlan;

	if (if_cmd) {
		char *name = canonical_if_name(nsdev);
		fprintf(out, "!\ninterface %s\n", name);
		free(name);
	}

	switch (nsdev->type) {
	case SW_IF_SWITCHED:
	case SW_IF_ROUTED:
		status = sw_ops->if_get_cfg(sw_ops, nsdev->ifindex, &flags,
				&access_vlan, bmp);
		if (status == -1) {
			EX_STATUS_PERROR(ctx, "get interface config failed");
			ret = CLI_EX_WARNING;
			goto next;
		}

		/* description */
		if(!switch_get_if_desc(nsdev->ifindex, desc) && strlen(desc))
			fprintf(out, " description %s\n", desc);
	}
	
	if (nsdev->type == SW_IF_SWITCHED) {
		/* switchport access vlan */
		if(access_vlan != 1)
			fprintf(out, " switchport access vlan %d\n",
					access_vlan);
		/* switchport trunk allowed vlan */
		do {
			int i;
			if((bmp[0] | 0x01) != 0x01)
				break;
			if((bmp[SW_VLAN_BMP_NO - 1] | 0xf0) != 0xf0)
				break;
			for(i = SW_VLAN_BMP_NO - 2; i > 0 && !bmp[i]; i--);
			if(i)
				break;
			need_trunk_vlans = 0;
		} while(0);
		if(need_trunk_vlans)
			list_vlans(out, bmp);
		/* switchport mode */
		if(flags & IF_MODE_ACCESS)
			fprintf(out, " switchport mode access\n");
		if(flags & IF_MODE_TRUNK)
			fprintf(out, " switchport mode trunk\n");
	}

	/* FIXME perhaps these need to be rewritten using ethtool suff;
	 * is there any way to determine if a speed/duplex was enforced or
	 * auto-negociated? it's not ok to issue a "speed 10" command
	 * if the speed was auto-negociated. Ionut, any thoughts on this?
	if (nsdev->type == SW_IF_SWITCHED || nsdev->type == SW_IF_ROUTED) {
		/ * speed * /
		if(swcfgr.ext.cfg.speed != SW_SPEED_AUTO) {
			char *speed = NULL;
			switch(swcfgr.ext.cfg.speed) {
			case SW_SPEED_10:
				speed = "10";
				break;
			case SW_SPEED_100:
				speed = "100";
				break;
			case SW_SPEED_1000:
				speed = "1000";
				break;
			}
			fprintf(out, " speed %s\n", speed);
		}
		/ * duplex * /
		if(swcfgr.ext.cfg.duplex != SW_DUPLEX_AUTO) {
			char *duplex = NULL;
			switch(swcfgr.ext.cfg.duplex) {
			case SW_DUPLEX_HALF:
				duplex = "half";
				break;
			case SW_DUPLEX_FULL:
				duplex = "full";
				break;
			}
			fprintf(out, " duplex %s\n", duplex);
		}
	}
	*/

	/* per interface cdp status (enabled/disabled). by default cdp is enabled. */
next:
	if (CDP_SESSION_OPEN(ctx, cdp)) {
		if (!cdp_is_enabled(cdp, nsdev->ifindex))
			fprintf(out, " no cdp enable\n");
		CDP_SESSION_CLOSE(ctx, cdp);
	}

	/* ip address */
	do {
		LIST_HEAD(addrl);
		struct if_addr *if_addr;
		struct ifreq ifr;
		int ins = socket(AF_INET, SOCK_DGRAM, 0);
		struct in_addr mask;

		assert(ins != -1);

		if (nsdev->type != SW_IF_VIF && nsdev->type != SW_IF_ROUTED)
			break;
		if (if_get_addr(nsdev->ifindex, AF_INET, &addrl, NULL))
			break;
		if (list_empty(&addrl)) {
			fprintf(out, " no ip address\n");
			break;
		}

		assert(strlen(nsdev->name) < IFNAMSIZ);
		strcpy(ifr.ifr_name, nsdev->name);

		status = ioctl(ins, SIOCGIFADDR, &ifr);
		assert(!status);

		/* we can't tell the prefix length from SIOCGIFADDR result and we
		 * don't know if primary address comes first in addrl, so walk
		 * addrl twice: first time print primary address, then dump all
		 * other addresses
		 */

		list_for_each_entry(if_addr, &addrl, lh) {
			if (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr != if_addr->inet.s_addr)
				continue;
			mask.s_addr = nbit2mask(if_addr->prefixlen);
			fprintf(out, " ip address %s", inet_ntoa(if_addr->inet));
			fprintf(out, " %s\n", inet_ntoa(mask));
		}

		list_for_each_entry(if_addr, &addrl, lh) {
			if (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr == if_addr->inet.s_addr)
				continue;
			mask.s_addr = nbit2mask(if_addr->prefixlen);
			fprintf(out, " ip address %s", inet_ntoa(if_addr->inet));
			fprintf(out, " %s secondary\n", inet_ntoa(mask));
		}

		close(ins);
		list_free(&addrl, struct if_addr, lh);
	} while (0);

	return ret;
}

struct write_enable_secret_priv {
	FILE *out;
	int level;
};

static int write_enable_secret(char *pw, void *__priv)
{
	struct write_enable_secret_priv *priv = __priv;

	if (*pw == '\0')
		return 0;

	if (priv->level == SW_MAX_ENABLE)
		fprintf(priv->out, "enable secret 5 %s\n", pw);
	else
		fprintf(priv->out, "enable secret level %d 5 %s\n", priv->level, pw);
	return 0;
}

/*
 * tagged_if
 *     Whether to open a separate config file for tagged interfaces (1)
 *     or use the already provided stream in *out (0).
 */
int build_config_global(struct cli_context *ctx, FILE *out, int tagged_if)
{
	struct if_map if_map;
	struct cdp_configuration cdp;
	int status, i, j, sock_fd;
	char hostname[128];
	unsigned char vlans[SW_VLAN_BMP_NO];
	int ret = CLI_EX_OK;
	char vlan_name[SW_MAX_VLAN_NAME + 1], def_name[SW_MAX_VLAN_NAME + 1];
	int igmp_snooping;
	int age_time = SW_DEFAULT_AGE_TIME;
	int mac_type, ifindex, vlan;
	unsigned char mac_addr[ETH_ALEN];
	struct net_switch_mac_e *mac, *mac_tmp;
	struct list_head macs;

	INIT_LIST_HEAD(&macs);
	if_map_init(&if_map);

	SW_SOCK_OPEN(ctx, sock_fd);

	/* hostname */
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0'; /* paranoia :P */
	fprintf(out, "!\nhostname %s\n", hostname);

	/* enable secrets */
	fputs("!\n", out);
	for(i = 1; i <= SW_MAX_ENABLE; i++) {
		struct write_enable_secret_priv priv = {
			.out = out,
			.level = i
		};

		switch_auth(SHARED_AUTH_ENABLE, i, write_enable_secret, &priv);
	}

	/* TODO: IGMP snooping static mrouter configuration */
	fprintf(out, "!\n");

	/* IGMP snooping global flag */
	status = sw_ops->igmp_get(sw_ops, NULL, &igmp_snooping);
	if (status != 0) {
		EX_STATUS_PERROR(ctx, "igmp_get failed");
		ret = CLI_EX_WARNING;
		goto vlans;
	}
	if (igmp_snooping) {
		unsigned char map[SW_VLAN_BMP_NO];

		status = sw_ops->igmp_get(sw_ops, (void *)&map[0], &igmp_snooping);
		if (status != 0) {
			EX_STATUS_PERROR(ctx, "igmp_get failed");
			ret = CLI_EX_WARNING;
			goto vlans;
		}

		for (i = SW_MIN_VLAN; i <= SW_MAX_VLAN; i++) {
			if (sw_bitmap_test(map, i))
				fprintf(out, "no ip igmp snooping vlan %d\n", i);
		}
	} else {
		fprintf(out, "no ip igmp snooping\n");
	}

vlans:
	/* vlans (aka replacement for vlan database) */
	status = sw_ops->get_vdb(sw_ops, vlans);
	if (status) {
		fprintf(out, "get_vdb failed\n");
		ret = CLI_EX_WARNING;
	}
	else
		for (i = SW_MIN_VLAN, j = 0; i < SW_MAX_VLAN; i++) {
			if (!sw_bitmap_test(vlans, i))
				continue;
			if (sw_is_default_vlan(i))
				continue;
			fprintf(out, "!\nvlan %d\n", i);
			switch_get_vlan_desc(i, vlan_name);
			__default_vlan_name(def_name, i);
			if (strcmp(vlan_name, def_name))
				fprintf(out, " name %s\n", vlan_name);
			j++;
		}

	/* physical interfaces and VIFs */
	status = if_map_fetch(&if_map, SW_IF_SWITCHED | SW_IF_ROUTED | SW_IF_VIF);
	if (status) {
		EX_STATUS_PERROR(ctx, "if_map_fetch failed");
		ret = CLI_EX_WARNING;
		goto static_mac;
	}

	struct list_head *iter, *tmp;
	struct net_switch_device *dev;
	list_for_each_safe(iter, tmp, &if_map.dev) {
		FILE *if_out = out;
		char tag[SW_MAX_TAG + 1];
		char path[PATH_MAX];
		int if_cmd = 1;
		dev = list_entry(iter, struct net_switch_device, lh);

		if (tagged_if && !switch_get_if_tag(dev->ifindex, tag)) {
			status = snprintf(path, sizeof(path), "%s/%s", SW_TAGS_FILE, tag);
			assert (status < sizeof(path)); // FIXME

			if_out = fopen(path, "w+");
			assert(if_out != NULL); // FIXME

			if_cmd = 0;
		}
		build_config_interface(ctx, if_out, dev, if_cmd);
		fprintf(out, "!\n");
		if (if_out != out)
			fclose(if_out);
	}

static_mac:
	/* static macs */
	status = if_map_init_ifindex_hash(&if_map);
	assert(!status);

	init_mac_filter(&ifindex, &vlan, &mac_type, mac_addr);
	mac_type = SW_FDB_MAC_STATIC;

	status = sw_ops->get_mac(sw_ops, ifindex, vlan, mac_type, mac_addr, &macs);
	if (status < 0) {
		EX_STATUS_PERROR(ctx, "failed to get mac addresses");
		ret = CLI_EX_WARNING;
		goto aging;
	}

	list_for_each_entry_safe(mac, mac_tmp, &macs, lh) {
		unsigned char *addr = &mac->addr[0];
		char *if_name = canonical_if_name(if_map_lookup_ifindex(&if_map, mac->ifindex, sock_fd));

		fprintf(out, "mac-address-table static "
				"%02hhx%02hhx.%02hhx%02hhx.%02hhx%02hhx "
				"vlan %d interface %s\n",
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
				mac->vlan, if_name);
		free(if_name);
		list_del(&mac->lh);
		free(mac);
	}
	if_map_cleanup(&if_map);

aging:
	/* fdb mac aging time */
	status = sw_ops->get_age_time(sw_ops, &age_time);
	if (status < 0) {
		EX_STATUS_PERROR(ctx, "get age time failed");
		ret = CLI_EX_WARNING;
	}
	if (age_time != SW_DEFAULT_AGE_TIME)
		fprintf(out, "mac-address-table aging-time %d\n!\n", age_time);

	/* cdp global settings */
	switch_get_cdp(&cdp);
	if (!cdp.enabled)
		fprintf(out, "no cdp run\n");
	if (cdp.version != CDP_DFL_VERSION)
		fprintf(out, "no cdp advertise-v2\n");
	if (cdp.timer != CDP_DFL_TIMER)
		fprintf(out, "cdp timer %d\n", cdp.timer);
	if (cdp.holdtime != CDP_DFL_HOLDTIME)
		fprintf(out, "cdp holdtime %d\n", cdp.holdtime);

	/* line vty stuff TODO */

	SW_SOCK_CLOSE(ctx, sock_fd);
	return ret;
}

static __inline__ int __cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out, *tmp_out;
	struct net_switch_device nsdev = {
		.ifindex = 0
	};
	int tmp_fd, sock_fd;
	char tmp_name[] = "/tmp/swcli.XXXXXX\0";
	int ret = CLI_EX_OK;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (argc >= 3 && !strcmp(nodev[2]->name, "interface")) {
		/* show config for a specific interface */
		int iftype, ifvlan;

		SHIFT_ARG(argc, argv, nodev, 3);
		assert(argc >= 2);
		if_args_to_ifindex(ctx, argv, nodev, sock_fd, nsdev.ifindex, iftype, nsdev.name);
		if_get_type(ctx, sock_fd, nsdev.ifindex, nsdev.name, iftype, ifvlan);
		nsdev.type = iftype;
	}

	tmp_fd = mkstemp(tmp_name);
	if (tmp_fd == -1) {
		EX_STATUS_PERROR(ctx, "mkstemp failed");
		SW_SOCK_CLOSE(ctx, sock_fd);
		return CLI_EX_REJECTED;
	}

	tmp_out = fdopen(tmp_fd, "w+");

	out = ctx->out_open(ctx, 1);
	fprintf(out, "Building configuration...\n");
	fflush(out);

	ret = nsdev.ifindex ? build_config_interface(ctx, tmp_out, &nsdev, 1) :
		build_config_global(ctx, tmp_out, 0);

	if (!ret) {
		unsigned char buf[4096];
		size_t nmemb;

		fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(tmp_out));
		rewind(tmp_out);
		while((nmemb = fread(buf, 1, sizeof(buf), tmp_out)))
			fwrite(buf, nmemb, 1, out);
		fprintf(out, "end\n\n");
		fflush(out);
	}

	fclose(tmp_out);
	unlink(tmp_name);
	fclose(out);
	SW_SOCK_CLOSE(ctx, sock_fd);
	return ret;
}

int cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int ret, sock_fd, save_fd = SWCLI_CTX(ctx)->sock_fd;

	SW_SOCK_OPEN(ctx, sock_fd);
	SWCLI_CTX(ctx)->sock_fd = sock_fd;
	/* This is a trick to always prevent downstack functions from
	 * opening a new sock_fd or closing the existing one */

	ret = __cmd_show_run(ctx, argc, argv, nodev);

	SWCLI_CTX(ctx)->sock_fd = save_fd;
	SW_SOCK_CLOSE(ctx, sock_fd);

	return ret;
}

int cmd_write_mem(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *cfg, *out;
	int ret, sock_fd, save_fd = SWCLI_CTX(ctx)->sock_fd;
	struct stat sb;

	SW_SOCK_OPEN(ctx, sock_fd);
	SWCLI_CTX(ctx)->sock_fd = sock_fd;
	/* This is a trick to always prevent downstack functions from
	 * opening a new sock_fd or closing the existing one */

	out = ctx->out_open(ctx, 0);

	if(lstat(SW_CONFIG_ROOT, &sb) == -1) {
		fprintf(out, "config root '%s' not found, attempting to create it... ", SW_CONFIG_ROOT);
		fflush(out);
		if(mkdir(SW_CONFIG_ROOT, S_IRWXU) == -1) {
				EX_STATUS_PERROR(ctx, "\ncreate dir failed ");
				return CLI_EX_REJECTED;
		}
		fprintf(out, "done\n");
		fflush(out);
	} else if(!S_ISDIR(sb.st_mode)) {
				EX_STATUS_PERROR(ctx, "config root '"SW_CONFIG_ROOT"' is not a dir! ");
				return CLI_EX_REJECTED;
	}

	cfg = fopen(SW_CONFIG_FILE, "w+");
	if (cfg == NULL) {
		EX_STATUS_PERROR(ctx, "fopen failed");
		return CLI_EX_REJECTED;
	}

	fprintf(out, "Building configuration...\n");
	fflush(out);

	ret = build_config_global(ctx, cfg, 1);

	SWCLI_CTX(ctx)->sock_fd = save_fd;
	SW_SOCK_CLOSE(ctx, sock_fd);
	fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(cfg));
	fclose(cfg);

	return ret;
}

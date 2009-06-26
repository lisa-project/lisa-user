#include "swcli.h"

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

static char *canonical_if_name(struct net_switch_dev *nsdev)
{
	char *ret = NULL;
	int n, status = -1;

	if (nsdev == NULL)
		return NULL;

	switch (nsdev->type) {
	case SW_IF_SWITCHED:
	case SW_IF_ROUTED:
		if ((n = if_parse_ethernet(nsdev->name)) >= 0)
			status = asprintf(&ret, "Ethernet %d", n);
		else
			status = asprintf(&ret, "netdev %s", nsdev->name);
		break;
	case SW_IF_VIF:
		status = asprintf(&ret, "vlan %d", nsdev->vlan);
		break;
	}

	return status == -1 ? NULL : ret;
}

int build_config_interface(struct cli_context *ctx, FILE *out, struct net_switch_dev *nsdev, int if_cmd)
{
	int sock_fd, status;
	unsigned char bmp[SW_VLAN_BMP_NO];
	char desc[SW_MAX_PORT_DESC];
	int need_trunk_vlans = 1;
	struct swcfgreq swcfgr;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (if_cmd) {
		char *name = canonical_if_name(nsdev);
		fprintf(out, "!\ninterface %s\n", name);
		free(name);
	}

	switch (nsdev->type) {
	case SW_IF_SWITCHED:
	case SW_IF_ROUTED:
		swcfgr.cmd = SWCFG_GETIFCFG;
		swcfgr.ifindex = nsdev->ifindex;
		swcfgr.ext.cfg.forbidden_vlans = bmp;
		swcfgr.ext.cfg.description = desc;
		status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
		// FIXME status == -1  --> errno

		/* description */
		if(strlen(desc))
			fprintf(out, " description %s\n", desc);
	}
	
	if (nsdev->type == SW_IF_SWITCHED) {
		/* switchport access vlan */
		if(swcfgr.ext.cfg.access_vlan != 1)
			fprintf(out, " switchport access vlan %d\n",
					swcfgr.ext.cfg.access_vlan);
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
		if(swcfgr.ext.cfg.flags & SW_PFL_ACCESS)
			fprintf(out, " switchport mode access\n");
		if(swcfgr.ext.cfg.flags & SW_PFL_TRUNK)
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

	SW_SOCK_CLOSE(ctx, sock_fd);
	return CLI_EX_OK;
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
	int status, i, j, sock_fd;
	char hostname[128];
	struct swcfgreq swcfgr;

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

		shared_auth(SHARED_AUTH_ENABLE, i, write_enable_secret, &priv);
	}

	/* vlans (aka replacement for vlan database) */
	swcfgr.cmd = SWCFG_GETVDB;
	swcfgr.vlan = 0;
	swcfgr.ext.vlan_desc = NULL;
	status = buf_alloc_swcfgr(&swcfgr, sock_fd);
	assert(status > 0); // FIXME

	status /= sizeof(struct net_switch_vdb);
	for (i = 0, j = 0; i < status; i++) {
		struct net_switch_vdb *entry =
			(struct net_switch_vdb *)swcfgr.buf.addr + i;
		char vlan_name[9];

		if(sw_is_default_vlan(entry->vlan))
			continue;
		fprintf(out, "!\nvlan %d\n", entry->vlan);
		__default_vlan_name(vlan_name, entry->vlan);
		if (strcmp(entry->name, vlan_name))
			fprintf(out, " name %s\n", entry->name);
		j++;
	}
#ifdef USE_EXIT_IN_CONF
	if (j)
		fprintf(out, "exit\n");
#endif

	/* physical interfaces and VIFs */
	status = if_map_fetch(&if_map, SW_IF_SWITCHED | SW_IF_ROUTED | SW_IF_VIF, sock_fd);
	assert(!status); // FIXME if not null, status is an error code (i.e. errno) so we can use EX_STATUS_PERROR

	for (i = 0; i < if_map.size; i++) {
		FILE *if_out = out;
		char tag[SW_MAX_TAG + 1];
		char path[PATH_MAX];

		if (tagged_if && !shared_get_if_tag(if_map.dev[i].ifindex, tag)) {
			status = snprintf(path, sizeof(path), "%s/%s", SW_TAGS_FILE, tag);
			assert (status < sizeof(path)); // FIXME

			if_out = fopen(path, "w+");
			assert(if_out != NULL); // FIXME
		}
		build_config_interface(ctx, if_out, &if_map.dev[i], !tagged_if);
		fprintf(out, "!\n");
		if (if_out != out)
			fclose(if_out);
	}

	/* static macs */
	status = if_map_init_ifindex_hash(&if_map);
	assert(!status);

	init_mac_filter(&swcfgr);
	swcfgr.cmd = SWCFG_GETMAC;
	swcfgr.ext.mac.type = SW_FDB_MAC_STATIC;

	status = buf_alloc_swcfgr(&swcfgr, sock_fd);
	assert(status >= 0); // FIXME

	status /= sizeof(struct net_switch_mac);
	for (i = 0; i < status; i++) {
		struct net_switch_mac *mac =
			(struct net_switch_mac *)swcfgr.buf.addr + i;
		unsigned char *addr = &mac->addr[0];
		char *if_name = canonical_if_name(if_map_lookup_ifindex(&if_map, mac->ifindex, sock_fd));

		fprintf(out, "mac-address-table static "
				"%02hhx%02hhx.%02hhx%02hhx.%02hhx%02hhx "
				"vlan %d interface %s\n",
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
				mac->vlan, if_name);
		free(if_name);
	}
	free(swcfgr.buf.addr);
	if_map_cleanup(&if_map);

	/* fdb mac aging time */
	swcfgr.cmd = SWCFG_GETAGETIME;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	assert(status != -1);
	if (swcfgr.ext.nsec != SW_DEFAULT_AGE_TIME)
		fprintf(out, "mac-address-table aging-time %d\n!\n", swcfgr.ext.nsec);

	/* cdp global settings TODO */

	/* line vty stuff TODO */

	SW_SOCK_CLOSE(ctx, sock_fd);
	free(if_map.dev);
	return CLI_EX_OK;
}

static __inline__ int __cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out, *tmp_out;
	int iftype;
	struct net_switch_dev nsdev = {
		.ifindex = 0
	};
	int tmp_fd, sock_fd;
	char tmp_name[] = "/tmp/swcli.XXXXXX\0";
	int ret = CLI_EX_OK;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (argc >= 3 && !strcmp(nodev[2]->name, "interface")) {
		/* show config for a specific interface */
		struct swcfgreq swcfgr;

		SHIFT_ARG(argc, argv, nodev, 3);
		assert(argc >= 2);
		if_args_to_ifindex(ctx, argv, nodev, sock_fd, nsdev.ifindex, iftype, nsdev.name);
		if_get_type(ctx, sock_fd, nsdev.ifindex, nsdev.name, swcfgr);
		nsdev.type = swcfgr.ext.switchport;
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

	cfg = fopen(SW_CONFIG_FILE, "w+");
	if (cfg == NULL) {
		EX_STATUS_PERROR(ctx, "fopen failed");
		return CLI_EX_REJECTED;
	}

	SW_SOCK_OPEN(ctx, sock_fd);
	SWCLI_CTX(ctx)->sock_fd = sock_fd;
	/* This is a trick to always prevent downstack functions from
	 * opening a new sock_fd or closing the existing one */

	out = ctx->out_open(ctx, 0);
	fprintf(out, "Building configuration...\n");
	fflush(out);

	ret = build_config_global(ctx, cfg, 1);

	SWCLI_CTX(ctx)->sock_fd = save_fd;
	SW_SOCK_CLOSE(ctx, sock_fd);
	fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(cfg));
	fclose(cfg);

	return ret;
}

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

int build_config_interface(struct cli_context *ctx, FILE *out, struct net_switch_dev *nsdev, int if_cmd)
{
	int sock_fd, status;
	unsigned char bmp[SW_VLAN_BMP_NO];
	char desc[SW_MAX_PORT_DESC];
	int need_trunk_vlans = 1;
	struct swcfgreq swcfgr;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (if_cmd) {
		int n;

		switch (nsdev->type) {
		case SW_IF_SWITCHED:
		case SW_IF_ROUTED:
			if ((n = if_parse_ethernet(nsdev->name)) >= 0)
				fprintf(out, "!\ninterface Ethernet %d\n", n);
			else
				fprintf(out, "!\ninterface netdev %s\n", nsdev->name);
			break;
		case SW_IF_VIF:
			fprintf(out, "!\ninterface vlan %d\n", nsdev->vlan);
			break;
		}
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

int build_config_global(struct cli_context *ctx, FILE *out, int sock_fd)
{
	struct if_map if_map;
	int status, i;

	if_map_init(&if_map);

	SW_SOCK_OPEN(ctx, sock_fd);


	/* physical interfaces and VIFs */
	status = if_map_fetch(&if_map, SW_IF_SWITCHED | SW_IF_ROUTED | SW_IF_VIF, sock_fd);
	assert(!status); // FIXME if not null, status is an error code (i.e. errno) so we can use EX_STATUS_PERROR

	for (i = 0; i < if_map.size; i++) {
		build_config_interface(ctx, out, &if_map.dev[i], 1);
		fprintf(out, "!\n");
	}


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
		build_config_global(ctx, tmp_out, sock_fd);

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

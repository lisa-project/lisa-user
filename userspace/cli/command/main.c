#include "swcli.h"
#include "main.h"

extern struct menu_node config_main;

int swcli_output_modifiers_run(struct cli_context *ctx, int argc, char **argv,
		struct menu_node **nodev)
{
	struct cli_filter_priv priv;
	int i, j, err = 0;

	for (i=argc-1; i>=0; i--) {
		if (!nodev[i]->run)
			continue;
		if (nodev[i]->run != swcli_output_modifiers_run)
			break;
	}

	/* check that a valid node was found */
	if (i < 0 || i >= argc-3 || strcmp(argv[i+1], "|")) {
		err = -EINVAL;
		goto out_return;
	}

	/* make i the index of the next node after '|' */
	i+=2;

	memset(&priv, 0, sizeof(priv));

	priv.argv = (const char **)calloc(argc - i + 2, sizeof(char *));
	if (!priv.argv) {
		err = -ENOMEM;
		goto out_return;
	}

	/* copy the necessary argv elements: filter type and verbatim filter
	 * arguments.
	 * argv[0] is intentionally left NULL. It will be filled in later by
	 * the cli.
	 */
	priv.argv[1] = nodev[i]->name;
	for (j = i+1; j<argc; j++)
		priv.argv[j - i + 1] = argv[j];

	/* set context information to be available for the output handler */
	ctx->filter.priv = &priv;
	ctx->filter.open = cli_filter_open;
	ctx->filter.close = cli_filter_close;

	/* the real handler knows only about arguments before the '|' */
	nodev[i-2]->run(ctx, i-1, argv, nodev);

	if (priv.argv)
		free(priv.argv);

out_return:
	return err;
}

static __inline__ void init_mac_filter(struct swcfgreq *swcfgr)
{
	swcfgr->ifindex = 0;
	memset(&swcfgr->ext.mac.addr, 0, ETH_ALEN);
	swcfgr->ext.mac.type = SW_FDB_ANY;
	swcfgr->vlan = 0;
}

static int parse_mac_filter(struct swcfgreq *swcfgr, struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev, int sock_fd, char *ifname)
{
	int status;

	init_mac_filter(swcfgr);

	if (!argc)
		return 0;

	do {
		if (!strcmp(nodev[0]->name, "static")) {
			swcfgr->ext.mac.type = SW_FDB_STATIC;
			SHIFT_ARG(argc, argv, nodev);
			break;
		}

		if (!strcmp(nodev[0]->name, "dynamic")) {
			swcfgr->ext.mac.type = SW_FDB_DYN;
			SHIFT_ARG(argc, argv, nodev);
			break;
		}
	} while (0);

	if (!argc)
		return 0;

	if (!strcmp(nodev[0]->name, "address")) {
		assert(argc >= 2);
		status = parse_mac(argv[1], swcfgr->ext.mac.addr);
		assert(!status);
		SHIFT_ARG(argc, argv, nodev, 2);
	}

	if (!argc)
		return 0;
	
	if (!strcmp(nodev[0]->name, "interface")) {
		char __name[IFNAMSIZ];
		int n;
		char *name = ifname ? ifname : &__name[0];

		assert(argc >= 3);
		SHIFT_ARG(argc, argv, nodev);

		status = if_parse_args(argv, nodev, name, &n);

		if (status == IF_T_ERROR) {
			if (n == -1)
				EX_STATUS_REASON(ctx, "invalid interface name");
			else 
				ctx->ex_status.reason = NULL;
			return CLI_EX_REJECTED;
		}
		
		if (!(swcfgr->ifindex = if_get_index(name, sock_fd))) {
			EX_STATUS_REASON(ctx, "interface %s does not exist", name);
			return CLI_EX_REJECTED;
		}

		SHIFT_ARG(argc, argv, nodev, 2);
	}

	if (!argc)
		return 0;

	if (!strcmp(nodev[0]->name, "vlan")) {
		assert(argc >= 2);
		swcfgr->vlan = atoi(argv[1]);
	}

	return 0;
}

int cmd_sh_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int ret = CLI_EX_OK;
	int status, sock_fd;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_GETMAC
	};
	char ifname[IFNAMSIZ];

	if (SW_SOCK_OPEN(ctx, sock_fd) == -1) {
		EX_STATUS_REASON(ctx, "%s", strerror(errno));
		return CLI_EX_REJECTED;
	}

	assert(argc >= 2);
	SHIFT_ARG(argc, argv, nodev, strcmp(nodev[1]->name, "mac") ? 2 : 3);

	if ((status = parse_mac_filter(&swcfgr, ctx, argc, argv, nodev, sock_fd, ifname))) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return status;
	}

	if ((status = buf_alloc_swcfgr(&swcfgr, sock_fd)) < 0)
		switch (-status) {
		case EINVAL:
			EX_STATUS_REASON(ctx, "interface %s not in switch", ifname);
			ret = CLI_EX_REJECTED;
			break;
		default:
			EX_STATUS_REASON_IOCTL(ctx, status);
			ret = CLI_EX_WARNING;
			break;
		}
	else {
		int size = status;
		struct if_map if_map;
		struct if_map_priv priv = {
			.map = &if_map,
			.sock_fd = sock_fd
		};

		if_map_init(&if_map);

		/* if user asked for mac on specific interface, all results will
		 * have the same ifindex and if_get_name fallback is enough;
		 * otherwise fetch interface list from kernel and init hash */
		if (!swcfgr.ifindex) {
			status = if_map_fetch(&if_map, SW_IF_SWITCHED, sock_fd);
			assert(!status);
			status = if_map_init_ifindex_hash(&if_map);
			assert(!status);
		}

		// FIXME open output
		print_mac(stdout, swcfgr.buf.addr, size, if_map_print_mac, &priv);

		if_map_cleanup(&if_map);
	}

	if (swcfgr.buf.addr != NULL)
		free(swcfgr.buf.addr);
	SW_SOCK_CLOSE(ctx, sock_fd);

	return ret;
}

int cmd_cl_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, sock_fd;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_DELMACDYN
	};

	if (SW_SOCK_OPEN(ctx, sock_fd) == -1) {
		EX_STATUS_REASON(ctx, "%s", strerror(errno));
		return CLI_EX_REJECTED;
	}

	assert(argc >= 2);
	SHIFT_ARG(argc, argv, nodev, strcmp(nodev[1]->name, "mac") ? 2 : 3);

	if ((status = parse_mac_filter(&swcfgr, ctx, argc, argv, nodev, sock_fd, NULL))) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return status;
	}

	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (status == -1) {
		// FIXME output
		fprintf(stdout, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(stdout);
	}

	return 0;
}

int cmd_conf_t(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *ctx = (void *)__ctx;

	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	ctx->cc.root = &config_main;
	ctx->enable_ctrl_z = 1;
	return 0;
}

int cmd_disable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_enable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_quit(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev) {
	struct rlshell_context *ctx = (void *)__ctx;

	ctx->exit = 1;
	swcli_dump_args(__ctx, argc, argv, nodev);
	return 0;
}

int cmd_help(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_ping(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_reload(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_history(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	int i;

	out = ctx->out_open(ctx, 1);
	for (i=0; i<1000; i++)
		fprintf(out, "%d\n", i);
	fflush(out);
	fclose(out);
	return 0;
}

int cmd_sh_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_addr(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_age(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_eth(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_priv(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	return CLI_EX_OK;
}

int cmd_sh_run_if(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_start(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_ver(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_trace(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_wrme(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}


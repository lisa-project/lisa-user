#include "swcli.h"
#include "exec.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <linux/ethtool.h>

extern struct menu_node config_main;

/* Dash stuff used for pretty printing of tables */
static const char *dash =
"---------------------------------------"
"---------------------------------------";
/* Dash length must match the number of dashes in the string above */
#define DASH_LENGTH 78
/* Get a null-terminated string (char *) of x dashes */
#define DASHES(x) (dash + DASH_LENGTH - (x))

#define MAX_COMMA_BUFFER_LEN 80
struct comma_buffer {
	char str[MAX_COMMA_BUFFER_LEN];
	int offset;
	int size;
};
#define COMMA_BUFFER_INIT(__size) { \
	.str = {'\0'}, \
	.offset = 0, \
	.size = (__size) \
}
static inline int comma_buffer_append(struct comma_buffer *buf, const char *str)
{
	size_t str_len = strlen(str);
	if (buf->offset + str_len + (buf->offset ? 2 : 0) >= buf->size)
		return -ENOMEM;
	if (buf->offset) {
		buf->str[buf->offset++] = ',';
		buf->str[buf->offset++] = ' ';
	}
	strcpy(&buf->str[buf->offset], str);
	buf->offset += str_len;
	return 0;
}
static inline int comma_buffer_reset(struct comma_buffer *buf, const char *str)
{
	size_t str_len = strlen(str);
	if (str_len >= buf->size)
		return -ENOMEM;
	strcpy(&buf->str[0], str);
	buf->offset = str_len;
	return 0;
}
static inline int cmpstr(const void *a, const void *b) {
	return strcmp(*(const char **) a, *(const char **)b);
}

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

static int parse_mac_filter(struct swcfgreq *swcfgr, struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev, int sock_fd, char *ifname)
{
	int status;

	init_mac_filter(swcfgr);

	if (!argc)
		return 0;

	do {
		if (!strcmp(nodev[0]->name, "static")) {
			swcfgr->ext.mac.type = SW_FDB_MAC_STATIC;
			SHIFT_ARG(argc, argv, nodev);
			break;
		}

		if (!strcmp(nodev[0]->name, "dynamic")) {
			swcfgr->ext.mac.type = SW_FDB_MAC_DYNAMIC;
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
		assert(argc >= 3);
		SHIFT_ARG(argc, argv, nodev);

		if_args_to_ifindex(ctx, argv, nodev, sock_fd, swcfgr->ifindex, status, ifname);
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
	FILE *out;
	int ret = CLI_EX_OK;
	int status, sock_fd;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_GETMAC
	};
	char ifname[IFNAMSIZ];

	SW_SOCK_OPEN(ctx, sock_fd);

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
			EX_STATUS_REASON_IOCTL(ctx, -status);
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
			status = if_map_fetch(&if_map, SW_IF_SWITCHED);
			assert(!status);
			status = if_map_init_ifindex_hash(&if_map);
			assert(!status);
		}

		out = ctx->out_open(ctx, 1);
		print_mac(out, swcfgr.buf.addr, size, if_map_print_mac, &priv);
		fflush(out);
		fclose(out);

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

	SW_SOCK_OPEN(ctx, sock_fd);

	assert(argc >= 2);
	SHIFT_ARG(argc, argv, nodev, strcmp(nodev[1]->name, "mac") ? 2 : 3);

	if ((status = parse_mac_filter(&swcfgr, ctx, argc, argv, nodev, sock_fd, NULL))) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return status;
	}

	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (status == -1) {
		EX_STATUS_REASON(ctx, "MAC address could not be removed\n"
				"Address not found\n\n");
		return CLI_EX_WARNING;
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

int cmd_quit(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev) {
	struct rlshell_context *ctx = (void *)__ctx;

	ctx->exit = 1;
	return 0;
}


int cmd_help(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;

	out = ctx->out_open(ctx, 0);
	fprintf(out,
		"Help may be requested at any point in a command by entering\n"
		"a question mark '?'.  If nothing matches, the help list will\n"
		"be empty and you must backup until entering a '?' shows the\n"
		"available options.\n"
		"Two styles of help are provided:\n"
		"1. Full help is available when you are ready to enter a\n"
		"   command argument (e.g. 'show ?') and describes each possible\n"
		"   argument.\n"
		"2. Partial help is provided when an abbreviated argument is entered\n"
		"   and you want to know what arguments match the input\n"
		"   (e.g. 'show pr?'.)\n\n"
			);
	fflush(out);
	return 0;
}

int cmd_history(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	HIST_ENTRY **history;
	HIST_ENTRY *entry;
	int i;

	out = ctx->out_open(ctx, 0);
	history = history_list();
	if (history) {
		for (i = 0; (entry = history[i]); i++) {
			fprintf(out, "   %s\n", entry->line);
		}
	}
	return 0;
}

int cmd_sh_clock(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	time_t rawtime;
	struct tm * timeinfo;

	out = ctx->out_open(ctx,0);
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	fprintf(out,"%s",asctime(timeinfo));
	fflush(out);

	return 0;
}

/*
 * FIXME: other options we may want to check ...
 *
 * Ethtool ioctls:
 * 		ETHTOOL_GTXCSUM
 * 		ETHTOOL_GSG
 * 		ETHTOOL_GTSO
 * 		ETHTOOL_GUFO
 * 		ETHTOOL_GGSO
 * 		ETHTOOL_GGRO
 *
 * Generic socket ioctls:
 *		SIOCGIFADDR
 *		SIOCGIFBRDADDR
 *		SIOCGIFNETMASK
 *		SIOCGIFMETRIC
 */
static int show_interfaces(struct cli_context *ctx, int sock_fd, struct if_map *map)
{
	struct ethtool_cmd settings;
	struct ethtool_drvinfo drv;
	struct ethtool_pauseparam pause;
	struct ethtool_value value;
	struct ethtool_perm_addr *epaddr;
	struct net_switch_device *dev;
	struct ifreq ifr;
	int nstats;
	FILE *out;

	out = ctx->out_open(ctx, 1);
	list_for_each_entry(dev, &map->dev, lh) {
		fprintf(out, "Interface %s, type=%d, index=%d\n",
				dev->name, dev->type, dev->ifindex);
		strcpy(ifr.ifr_name, dev->name);
		/* Link status */
		value.cmd = ETHTOOL_GLINK;
		ifr.ifr_data = &value;
		if (!ioctl(sock_fd, SIOCETHTOOL, &ifr)) {
			fprintf(out, "\tLink detected: %s\n", value.data? "yes" : "no");
		}
		/* Interface status */
		if (!ioctl(sock_fd, SIOCGIFFLAGS, &ifr))
			fprintf(out, "\tInterface status: %s\n", ifr.ifr_flags & IFF_UP? "up" : "down");
		/* Hardware address */
		if (!ioctl(sock_fd, SIOCGIFHWADDR, &ifr)) {
			unsigned char *buf = (unsigned char *)ifr.ifr_hwaddr.sa_data;
			fprintf(out, "\tHardware address: %02x:%02x:%02x:%02x:%02x:%02x\n",
					buf[0], buf[1],
					buf[2], buf[3],
					buf[4], buf[5]);
		}
		/* Permanent address */
		epaddr = (struct ethtool_perm_addr *)alloca(
				sizeof(struct ethtool_perm_addr) +  ETH_ALEN);
		epaddr->cmd = ETHTOOL_GPERMADDR;
		epaddr->size = ETH_ALEN;
		ifr.ifr_data = epaddr;
		if (!ioctl(sock_fd, SIOCETHTOOL, &ifr))
			fprintf(out, "\tPermanent address: %02x:%02x:%02x:%02x:%02x:%02x\n",
					epaddr->data[0], epaddr->data[1], epaddr->data[2],
					epaddr->data[3], epaddr->data[4], epaddr->data[5]);
		/* MTU */
		if (!ioctl(sock_fd, SIOCGIFMTU, &ifr))
			fprintf(out, "\tMTU: %d\n", ifr.ifr_mtu);
		/* Flow control parameters */
		pause.cmd = ETHTOOL_GPAUSEPARAM;
		ifr.ifr_data = &pause;
		if (!ioctl(sock_fd, SIOCETHTOOL, &ifr))
			fprintf(out, "\tPause param: autoneg: %d, rx_pause: %d, tx_pause: %d\n",
					pause.autoneg, pause.rx_pause, pause.tx_pause);
		/* Interface settings */
		if (!if_get_settings(dev->ifindex, sock_fd, &settings)) {
			fprintf(out, "\tSupported = 0x%04x\n"
					"\tAdvertising = 0x%04x\n"
					"\tSpeed = %d\n"
					"\tDuplex = %d\n"
					"\tAuto = %d\n",
					settings.supported,
					settings.advertising,
					settings.speed,
					settings.duplex,
					settings.autoneg);
		}
		/* Driver information */
		drv.cmd = ETHTOOL_GDRVINFO;
		ifr.ifr_data = &drv;
		nstats = -1;
		if (!ioctl(sock_fd, SIOCETHTOOL, &ifr)) {
			fprintf(out, "\tDriver: %s, version: %s, fw_version: %s, bus: %s\n",
					drv.driver, drv.version, drv.fw_version, drv.bus_info);
			nstats = drv.n_stats;
		}
		/* NIC specific statistics counters */
		do {
			struct ethtool_stats *hwstats;
			struct ethtool_gstrings *strings;
			int j;

			if (nstats < 0)
				break;
			/* Get hardware specific statistics strings */
			strings = (struct ethtool_gstrings *)alloca(
					sizeof(struct ethtool_gstrings) + nstats * ETH_GSTRING_LEN);
			strings->cmd = ETHTOOL_GSTRINGS;
			strings->string_set = ETH_SS_STATS;
			ifr.ifr_data = strings;
			if (ioctl(sock_fd, SIOCETHTOOL, &ifr)) {
				fprintf(out, "ETHTOOL_GSTRINGS failed: %s\n", strerror(errno));
				break;
			}

			/* Hardware specific statistics values */
			hwstats = (struct ethtool_stats *)alloca(
					sizeof(struct ethtool_stats) + nstats * sizeof(uint64_t));
			hwstats->cmd = ETHTOOL_GSTATS;
			ifr.ifr_data = hwstats;
			if (ioctl(sock_fd, SIOCETHTOOL, &ifr)) {
				fprintf(out, "ETHTOOL_GSTATS failed: %s\n", strerror(errno));
				break;
			}

			/* Display statistics values */
			fprintf(out, "\tNIC statistics:\n");
			for (j=0; j<nstats; j++) {
				fprintf(out, "\t\t%.*s: %llu\n",
						ETH_GSTRING_LEN,
						&strings->data[j * ETH_GSTRING_LEN],
						hwstats->data[j]);
			}
		} while (0);

	}
	fclose(out);

	return 0;
}

int cmd_sh_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int sock_fd, err = CLI_EX_OK;
	struct net_switch_device *dev;
	struct if_map if_map;
	struct swcfgreq swcfgr;

	SW_SOCK_OPEN(ctx, sock_fd);

	if_map_init(&if_map);
	if (argc > 2) {
		dev = malloc(sizeof(struct net_switch_device));
		if (!dev) {
			EX_STATUS_REASON(ctx, "alloc device failed\n");
			return CLI_EX_REJECTED;
		}

		SHIFT_ARG(argc, argv, nodev, 2);
		if_args_to_ifindex(ctx, argv, nodev, sock_fd, dev->ifindex,
				dev->type, dev->name);
		if_get_type(ctx, sock_fd, dev->ifindex, dev->name, swcfgr);
		dev->type = swcfgr.ext.switchport;
		dev->vlan = swcfgr.vlan;
		list_add_tail(&dev->lh, &if_map.dev);
	}
	else
		if_map_fetch(&if_map, SW_IF_SWITCHED | SW_IF_ROUTED | SW_IF_VIF);

	err = show_interfaces(ctx, sock_fd, &if_map);

	SW_SOCK_CLOSE(ctx, sock_fd);
	if_map_cleanup(&if_map);

	return err;
}

int cmd_sh_ip_igmps(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{

	FILE *out;
	char name[10];

	strcpy(name,"");
	SHIFT_ARG(argc, argv, nodev,4);
	if(argc)
		strcpy(name,argv[1]);
	out = ctx->out_open(ctx, 1);
	fprintf(out,"IGMP_SNOOPING %s\n ", name);
	fflush(out);
	return 0;
}

int cmd_sh_ip_igmps_mrouter(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	int ret = CLI_EX_OK, sock_fd = -1;
	int vlan = 0;
	struct list_head mrouters;
	struct net_switch_mrouter_e *entry, *tmp;
	int status;

	const char *fmt1 = "%-4s    %s\n";
	const char *fmt2 = "%4d    %s(static)\n";
	char ifname[IFNAMSIZ];

	SHIFT_ARG(argc, argv, nodev, 5);
	if (argc) {
		assert(argc > 1);
		vlan = atoi(argv[1]);
	}
	out = ctx->out_open(ctx, 1);

	INIT_LIST_HEAD(&mrouters);
	status = sw_ops->mrouters_get(sw_ops, vlan, &mrouters);
	if (status < 0) {
		EX_STATUS_PERROR(ctx, "failed to get mrouters");
		ret = CLI_EX_REJECTED;
		goto close_out;
	}

	/* Open socket for if_get_name. */
	SW_SOCK_OPEN(ctx, sock_fd);

	fprintf(out, fmt1, "Vlan", "Ports");
	fprintf(out, fmt1, DASHES(4), DASHES(8));

	list_for_each_entry_safe(entry, tmp, &mrouters, lh) {
		if_get_name(entry->ifindex, sock_fd, ifname);
		fprintf(out, fmt2, entry->vlan, ifname);

		list_del(&entry->lh);
		free(entry);
	}
	SW_SOCK_CLOSE(ctx, sock_fd);

close_out:
	fclose(out);
	return ret;
}

/* IGMP group list entry. Each vlan has list of IGMP groups */
struct igmp_group_entry {
	uint32_t addr;
	int type;
	struct list_head lh;
	struct list_head interfaces;
};

/* Interface list entry. Each IGMP group has a list of interfaces
 * (either subscribers or mrouters).
 */
struct if_list_entry {
	int ifindex;
	struct list_head lh;
};

enum {
	IGMP_GROUP_DYNAMIC = 0,
	IGMP_GROUP_USER = 1,
	IGMP_GROUP_ANY = -1
};

int delete_igmp_groups(struct list_head *igmp_groups, int type)
{
	struct igmp_group_entry *group, *group_tmp;
	struct if_list_entry *ife, *ife_tmp;
	int i, count = 0;

	for (i=0; i<SW_MAX_VLAN; i++) {
		list_for_each_entry_safe(group, group_tmp, &igmp_groups[i], lh) {
			if (type != IGMP_GROUP_ANY && group->type != type)
				continue;
			list_for_each_entry_safe(ife, ife_tmp, &group->interfaces, lh) {
				list_del(&ife->lh);
				free(ife);
			}
			list_del(&group->lh);
			free(group);
			count++;
		}
	}

	return count;
}

int cmd_sh_ip_igmps_groups(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int ret = CLI_EX_OK;
	int i, status, sock_fd;
	struct list_head igmp_groups[SW_MAX_VLAN];
	struct igmp_group_entry *group;
	struct if_list_entry *ife;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_GETMAC,
		.ext.mac.type = SW_FDB_IGMP_ANY,
		.ifindex = 0,
		.vlan = 0
	};
	FILE *out;
	const char *fmt1 = "%-10s%-18s%-12s%-12s%s\n";
	const char *fmt2 = "%-10d%-18s%-12s%-12s%s\n";
	const char *fmt3 = "%48s%s\n";
	struct if_map if_map;
	int display_count = 0;
	int group_count = 0;
	const char *group_type = "";
	struct list_head mrouters;
	struct net_switch_mrouter_e *mrouter, *tmp;
	int vlan;

	INIT_LIST_HEAD(&mrouters);

	if (!strcmp(nodev[argc - 1]->name, "count")) {
		display_count = 1;
		argc--;
	}

	if_map_init(&if_map);

	SW_SOCK_OPEN(ctx, sock_fd);

	for (i=0; i<SW_MAX_VLAN; i++)
		INIT_LIST_HEAD(&igmp_groups[i]);

	/* Get the fdb entries corresponding to dynamic/user defined IGMP groups */
	if ((status = buf_alloc_swcfgr(&swcfgr, sock_fd)) < 0) {
		EX_STATUS_REASON_IOCTL(ctx, -status);
		ret = CLI_EX_WARNING;
		goto out_clean;
	}

#define igmp_group_addr(mac) (*(uint32_t *)&(mac)[2])
	status /= sizeof(struct net_switch_mac);
	for (i = 0; i<status; i++) {
		struct net_switch_mac *fdb_entry = (struct net_switch_mac *)swcfgr.buf.addr + i;
		uint32_t group_addr = igmp_group_addr(fdb_entry->addr);
		int found = 0;

		list_for_each_entry(group, &igmp_groups[fdb_entry->vlan], lh)
			if (group->addr == group_addr) {
				found = 1;
				break;
			}
		if (!found) {
			if (!(group = malloc(sizeof(struct igmp_group_entry)))) {
				EX_STATUS_REASON(ctx, strerror(ENOMEM));
				goto out_clean;
			}
			group->addr = group_addr;
			group->type = IGMP_GROUP_DYNAMIC;
			INIT_LIST_HEAD(&group->interfaces);
			list_add_tail(&group->lh, &igmp_groups[fdb_entry->vlan]);
			group_count++;
		}
		group->type = (fdb_entry->type & SW_FDB_STATIC) ? IGMP_GROUP_USER : group->type;
		if (!(ife = malloc(sizeof(struct if_list_entry)))) {
			EX_STATUS_REASON(ctx, strerror(ENOMEM));
			goto out_clean;
		}
		ife->ifindex = fdb_entry->ifindex;
		list_add_tail(&ife->lh, &group->interfaces);
	}

	/* Because of the nature of "static" groups (at least one static entry)
	 * we cannot use kernel filter on pseudo fdb entries. Moreover, if group
	 * is static, we must also display dynamic entries (interfaces). So
	 * build our group list, then apply filter ourselves if necessary.
	 */
	if (!strcmp(nodev[argc - 1]->name, "dynamic")) {
		group_type = " IGMP learned";
		group_count -= delete_igmp_groups(igmp_groups, IGMP_GROUP_USER);
	} else if (!strcmp(nodev[argc - 1]->name, "user")) {
		group_type = " static";
		group_count -= delete_igmp_groups(igmp_groups, IGMP_GROUP_DYNAMIC);
	}

	if (display_count) {
		out = ctx->out_open(ctx, 0);
		fprintf(out, "Total number of%s multicast groups: %d\n\n",
				group_type, group_count);
		goto out_clean;
	}

	/* If no pseudo fdb entries present, neither display anything,
	   nor query mrouter ports */
	if (!group_count)
		goto out_clean;

	free(swcfgr.buf.addr);

	/* Get the mrouters list */
	vlan = swcfgr.vlan;
	status = sw_ops->mrouters_get(sw_ops, vlan, &mrouters);
	if (status < 0) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		ret = CLI_EX_WARNING;
		goto out_clean;
	}

	list_for_each_entry_safe(mrouter, tmp, &mrouters, lh) {
		list_for_each_entry(group, &igmp_groups[mrouter->vlan], lh) {
			int found = 0;
			list_for_each_entry(ife, &group->interfaces, lh)
				if (ife->ifindex == mrouter->ifindex) {
					found = 1;
					break;
				}
			if (found)
				continue;
			if (!(ife = malloc(sizeof(struct if_list_entry)))) {
				EX_STATUS_REASON(ctx, strerror(ENOMEM));
				goto out_clean;
			}
			ife->ifindex = mrouter->ifindex;
			list_add_tail(&ife->lh, &group->interfaces);
		}
		list_del(&mrouter->lh);
		free(mrouter);
	}

	/* Build ifindex => ifname hash */
	status = if_map_fetch(&if_map, SW_IF_SWITCHED);
	if (status) {
		EX_STATUS_PERROR(ctx, "if_map_fetch failed");
		ret = CLI_EX_WARNING;
		goto out_clean;

	}
	status = if_map_init_ifindex_hash(&if_map);
	assert(!status);

	/* Print IGMP groups */
	out = ctx->out_open(ctx, 1);
	fprintf(out, fmt1, "Vlan", "Group", "Type", "Version", "Port List");
	fprintf(out, "%s\n", DASHES(63));
	for (i=0; i<SW_MAX_VLAN; i++) {
		list_for_each_entry(group, &igmp_groups[i], lh) {
			struct net_switch_device *nsdev;
			char *group_addr = inet_ntoa(*(struct in_addr*)&group->addr);
			int firstline = 1;
			struct comma_buffer buf = COMMA_BUFFER_INIT(24);
#define print_buf \
			do {\
				if (firstline) {\
					fprintf(out, fmt2, i, group_addr,\
							group->type == IGMP_GROUP_USER ? "user" : "igmp", "v2",\
							buf.str);\
				} else {\
					fprintf(out, fmt3, "", buf.str);\
				}\
			} while(0)
			list_for_each_entry(ife, &group->interfaces, lh) {
				nsdev = if_map_lookup_ifindex(&if_map, ife->ifindex, sock_fd);
				if (comma_buffer_append(&buf, nsdev->name)) {
					print_buf;
					firstline = 0;
					status = comma_buffer_reset(&buf, nsdev->name);
					assert(!status);
				}
			}
			print_buf;
#undef print_buf
		}
	}
	fclose(out);

out_clean:
	if_map_cleanup(&if_map);
	delete_igmp_groups(igmp_groups, IGMP_GROUP_ANY);
	if (swcfgr.buf.addr != NULL)
		free(swcfgr.buf.addr);
	SW_SOCK_CLOSE(ctx, sock_fd);
	list_for_each_entry_safe(mrouter, tmp, &mrouters, lh) {
		list_del(&mrouter->lh);
		free(mrouter);
	}

	return ret;
}

int cmd_sh_mac_age(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_GETAGETIME
	};
	int sock_fd, status;
	FILE *out;

	SW_SOCK_OPEN(ctx, sock_fd);
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	assert(status != -1);
	SW_SOCK_CLOSE(ctx, sock_fd);

	out = ctx->out_open(ctx, 0);
	fprintf(out, "%d\n", swcfgr.ext.nsec);

	return CLI_EX_OK;
}

int cmd_show_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int sock_fd = -1, status;
	int ret = CLI_EX_OK;
	const char *fmt1 = "%-4s %-32s %-9s %s\n";
	const char *fmt2 = "%-4d %-32s %-9s %s\n";
	const char *fmt3 = "%47s %s\n";
	FILE *out = NULL;
	struct if_map if_map;
	int i, j;
	unsigned char vlans[SW_VLAN_BMP_NO];
	char vlan_name[SW_MAX_VLAN_NAME + 1], *vlan_desc = NULL;
	int vlan_id = 0, min_vlan = SW_MIN_VLAN, max_vlan = SW_MAX_VLAN;

	assert(argc >= 2); /* show vlan */
	SHIFT_ARG(argc, argv, nodev, 2);

	if (argc && !strcmp(nodev[0]->name, "id")) {
		assert(argc >= 2);
		vlan_id = atoi(argv[1]);
		min_vlan = max_vlan = vlan_id;
	}

	if (argc && !strcmp(nodev[0]->name, "name")) {
		assert(argc >= 2);
		vlan_desc = argv[1];
	}

	SW_SOCK_OPEN(ctx, sock_fd);

	if_map_init(&if_map);
	status = if_map_fetch(&if_map, SW_IF_SWITCHED);
	if (status) {
		EX_STATUS_PERROR(ctx, "if_map_fetch failed");
		ret = CLI_EX_REJECTED;
		goto out_clean;
	}
	status = if_map_init_ifindex_hash(&if_map);
	assert(!status);

	status = sw_ops->get_vdb(sw_ops, vlans);
	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		ret = CLI_EX_REJECTED;
		goto out_clean;
	}

	if (vlan_id && !sw_bitmap_test(vlans, vlan_id)) {
		EX_STATUS_REASON(ctx, "VLAN id %d not found in current VLAN database\n", vlan_id);
		ret = CLI_EX_WARNING;
		goto out_clean;
	}

	if (vlan_desc != NULL) {
		for (i = min_vlan; i <= max_vlan; i++)
			if (sw_bitmap_test(vlans, i)) {
				status = sw_ops->get_vlan_desc(sw_ops, i, vlan_name);
				if (!status && !strcmp(vlan_name, vlan_desc))
					break;
			}
		if (i == max_vlan) {
			EX_STATUS_REASON(ctx, "VLAN %s not found in current VLAN database\n",
					vlan_desc);
			ret = CLI_EX_WARNING;
			goto out_clean;
		}
		else
			min_vlan = max_vlan = i;
	}

	out = ctx->out_open(ctx, 1);

	fprintf(out, fmt1, "VLAN", "Name", "Status", "Ports");
	fprintf(out, fmt1, DASHES(4), DASHES(32), DASHES(9), DASHES(31));

	for (i = min_vlan; i <= max_vlan; i++) {
		if (!sw_bitmap_test(vlans, i))
			continue;
		int vlan = i;
		struct swcfgreq vlif_swcfgr = {
			.cmd = SWCFG_GETVLANIFS,
			.vlan = vlan
		};
		int vlif_no = buf_alloc_swcfgr(&vlif_swcfgr, sock_fd);
		struct net_switch_device *nsdev;
		struct comma_buffer buf = COMMA_BUFFER_INIT(32);
		int firstline = 1;

		if (switch_get_vlan_desc(vlan, vlan_name))
			__default_vlan_name(vlan_name, vlan);
		/* FIXME kernel module should tell us whether vlan is "active"
		 * or "act/unsup" */
#define print_buf \
		do {\
			if (firstline) {\
				fprintf(out, fmt2, vlan, vlan_name,\
						vlan >= 1002 && vlan <= 1005 ? "act/unsup" : "active",\
						buf.str);\
			} else {\
				fprintf(out, fmt3, "", buf.str);\
			}\
		} while(0)

		/* if (vlif_no < 0) perror("getvlanif"); */
		assert(vlif_no >= 0);

		vlif_no /= sizeof(int);
		/* Sort interface names and print them comma separated. */
		char **names = (char**)malloc(vlif_no * sizeof(char*));
		assert(names != NULL);
		for (j = 0; j < vlif_no; j++) {
			int ifindex = ((int *)vlif_swcfgr.buf.addr)[j];

			nsdev = if_map_lookup_ifindex(&if_map, ifindex, sock_fd);
			assert(nsdev);

			names[j] = short_if_name(nsdev);
		}
		qsort(names, vlif_no, sizeof(char*), cmpstr);
		for (j = 0; j < vlif_no; j++) {
			if (comma_buffer_append(&buf, names[j])) {
				int tmp;

				print_buf;
				firstline = 0;
				tmp = comma_buffer_reset(&buf, names[j]);
				assert(!tmp);
			}
			free(names[j]);
		}
		print_buf;

		free(vlif_swcfgr.buf.addr);
		free(names);
#undef print_buf
	}

out_clean:
	if_map_cleanup(&if_map);
	if (sock_fd != -1)
		SW_SOCK_CLOSE(ctx, sock_fd);
	if (out != NULL)
		fclose(out);
	return ret;
}

int cmd_reload(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	int key;

	out = ctx->out_open(ctx, 0);
	fputs("Proceed with reload? [confirm]", out);
	fflush(out);
	key = read_key();
	if(key != 'y' && key != 'Y' && key != '\n') {
		fputc('\n', out);
		return 0;
	}
	system("reboot &> /dev/null");
	/* delay returning so that the prompt doesn't get displayed again */
	sleep(5);
	return 0;
}

int cmd_show_priv(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	uint32_t filter, priv;
	FILE *out;

	filter = (ctx->node_filter & PRIV_FILTER(PRIV_MAX)) + 1;
	for (priv = 0; filter != 1; filter >>= 1, priv++);
	out = ctx->out_open(ctx, 0);
	fprintf(out, "Current privilege level is %d\n", priv - 1);
	return 0;
}

/* FIXME: unimplemented commands */
int cmd_show_start(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_ver(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_disable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_enable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_ping(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_trace(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

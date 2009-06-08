#include "swcli.h"
#include "netlink.h"
#include "config_if.h"

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')
// FIXME move is_digit() to .h

extern struct menu_node menu_main;
extern struct menu_node config_main;
extern struct menu_node config_if_main;
extern struct menu_node config_vlan_main;
extern struct menu_node config_line_main;

static int parse_vlan_list(char *list, unsigned char *bmp)
{
	int state = 0;
	int min = 0, max;
	char *last = list, *ptr;

	memset(bmp, 0xff, SW_VLAN_BMP_NO);
	for(ptr = list; *ptr != '\0'; ptr++) {
		switch(state) {
		case 0: /* First number */
			if(is_digit(*ptr))
				continue;
			switch(*ptr) {
			case '-':
				min = atoi(last);
				if(sw_invalid_vlan(min))
					return 1;
				last = ptr + 1;
				state = 1;
				continue;
			case ',':
				min = atoi(last);
				if(sw_invalid_vlan(min))
					return 1;
				last = ptr + 1;
				sw_allow_vlan(bmp, min);
				continue;
			}
			return 2;
		case 1: /* Second number */
			if(is_digit(*ptr))
				continue;
			if(*ptr == ',') {
				max = atoi(last);
				if(sw_invalid_vlan(max))
					return 1;
				while(min <= max) {
					sw_allow_vlan(bmp, min);
					min++;
				}
				last = ptr + 1;
				state = 0;
			}
			return 2;
		}
	}
	switch(state) {
	case 0:
		min = atoi(last);
		if(sw_invalid_vlan(min))
			return 1;
		sw_allow_vlan(bmp, min);
		break;
	case 1:
		max = atoi(last);
		if(sw_invalid_vlan(max))
			return 1;
		while(min <= max) {
			sw_allow_vlan(bmp, min);
			min++;
		}
		break;
	}
	return 0;
}

int cmd_channel_group(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, sock_fd, ioctl_errno, logical_index = atoi(argv[1]);
	struct swcfgreq swcfgr;
	struct ifreq ifr;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	char *the_command, *bond_name;
	char *start_command = strdup("modprobe -o bond");
	char *end_command = strdup(" bonding mode=802.3ad miimon=100 lacp_rate=fast updelay=100 downdelay=100");

	char *if_name = calloc(IFNAMSIZ, 1);
	the_command = (char *) calloc(strlen(start_command) + 5 + strlen(end_command), sizeof(char));
	
	SW_SOCK_OPEN(ctx, sock_fd);

	if_get_name(uc->ifindex, sock_fd, if_name);

	SW_SOCK_OPEN(ctx, sock_fd);
	swcfgr.cmd = SWCFG_ISCHANNEL;
	swcfgr.channel_logical = logical_index;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (status)
		goto enslave;

	SW_SOCK_OPEN(ctx, sock_fd);
	swcfgr.cmd = SWCFG_ADDCHANNEL;
	swcfgr.channel_logical = logical_index;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (status)
	{
		EX_STATUS_REASON_IOCTL(ctx, ioctl_errno);
		return CLI_EX_REJECTED;
	};

	strcat(the_command, start_command);
	sprintf(the_command + strlen(the_command), "%d", swcfgr.channel_physical);
	strcat(the_command, end_command);
	
	system(the_command);
	// printf("%s\n", the_command);	

enslave:
	bond_name = (char *) calloc(10, sizeof(char));
	strcpy(bond_name, "bond");
	sprintf(bond_name + strlen(bond_name), "%d", swcfgr.channel_physical);
	
	strcpy(ifr.ifr_name, bond_name);

	SW_SOCK_OPEN(ctx, sock_fd);

	ioctl(sock_fd, SIOCGIFINDEX, &ifr);
	SW_SOCK_CLOSE(ctx, sock_fd);
	
	/*
	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	ctx->node_filter |= IFF_CHANNEL;
	ctx->root = &config_if_main;
	uc->ifindex = ifr.ifr_ifindex;
	uc->vlan = -1;
	*/
	
	use_if_ether(ctx, bond_name, ifr.ifr_ifindex, 1);

	memset(the_command, '\0', sizeof(the_command));

	strcpy(the_command, "ifenslave ");
	strcpy(the_command + strlen(the_command), bond_name);
	strcpy(the_command + strlen(the_command), " ");
	strcpy(the_command + strlen(the_command), if_name);

	system(the_command);
		
	return 0;
}

int cmd_cdp_if_set(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_session *cdp;
	struct cdp_configuration cfg;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	int enable = 1;
	int err;

	shared_get_cdp(&cfg);
	if (!cfg.enabled)
		return 0;

	if (!strcmp(nodev[0]->name, "no"))
		enable = 0;

	if (!CDP_SESSION_OPEN(ctx, cdp)) {
		EX_STATUS_REASON(ctx, "%s", strerror(errno));
		return CLI_EX_REJECTED;
	}
	err = cdp_set_interface(cdp, uc->ifindex, enable);
	CDP_SESSION_CLOSE(ctx, cdp);

	return err;
}

int cmd_if_desc(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcli_context *uc = SWCLI_CTX(ctx);
	struct swcfgreq swcfgr;
	int sock_fd;

	assert(argc);

	swcfgr.cmd = SWCFG_SETIFDESC;
	swcfgr.ifindex = uc->ifindex;
	if (strcmp(nodev[0]->name, "no")) {
		/* description is set/changed by user */
		assert(argc >= 2);
		swcfgr.ext.iface_desc = argv[1];
	} else {
		/* description is set to default */
		swcfgr.ext.iface_desc = (char *)"";
	}

	SW_SOCK_OPEN(ctx, sock_fd);
	ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd);

	return CLI_EX_OK;
}

int cmd_speed_duplex(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcli_context *uc = SWCLI_CTX(ctx);
	struct ethtool_cmd settings;
	int sock_fd;

	SW_SOCK_OPEN(ctx, sock_fd);

	/* Get current interface settings */
	if (if_get_settings(uc->ifindex, sock_fd, &settings)) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	/*
	 * Full/half duplex is forced and autonegotiation disabled unless the
	 * command is 'no duplex' or 'duplex auto'.
	 * Speed is forced to the specified value and autonegotiation disabled
	 * unless the command is 'no speed' or 'speed auto'.
	 */
	settings.autoneg = 1;
	if (strcmp(nodev[0]->name, "no") && strcmp(nodev[1]->name, "auto")) {
		settings.autoneg = 0;
		if (!strcmp(nodev[0]->name, "speed"))
			settings.speed = atoi(nodev[1]->name);
		else if (!strcmp(nodev[0]->name, "duplex"))
			settings.duplex = !strcmp(nodev[1]->name, "full");
	}

	/* Apply changed settings */
	if (if_set_settings(uc->ifindex, sock_fd, &settings)) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	SW_SOCK_CLOSE(ctx, sock_fd);

	return CLI_EX_OK;
}

int cmd_swport_off(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

int cmd_trunk_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{ 
	int parse_vlan_err = 0, i, sock_fd;
	struct swcfgreq swcfgr;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	unsigned char bmp[SW_VLAN_BMP_NO];

	assert(argc > 1);

	switch ((int)nodev[argc - 1]->priv) {
	case CMD_VLAN_SET:
		parse_vlan_err = parse_vlan_list(argv[argc - 1], bmp);
		swcfgr.cmd = SWCFG_SETTRUNKVLANS;
		break;
	case CMD_VLAN_ADD:
		parse_vlan_err = parse_vlan_list(argv[argc - 1], bmp);
		swcfgr.cmd = SWCFG_ADDTRUNKVLANS;
		break;
	case CMD_VLAN_ALL:
	case CMD_VLAN_NO:
		memset(bmp, 0, SW_VLAN_BMP_NO);
		swcfgr.cmd = SWCFG_SETTRUNKVLANS;
		break;
	case CMD_VLAN_EXCEPT:
		parse_vlan_err = parse_vlan_list(argv[argc - 1], bmp);
		for(i = 0; i < SW_VLAN_BMP_NO; i++)
			bmp[i] ^= 0xff;
		swcfgr.cmd = SWCFG_SETTRUNKVLANS;
		break;
	case CMD_VLAN_NONE:
		memset(bmp, 0xff, SW_VLAN_BMP_NO);
		swcfgr.cmd = SWCFG_SETTRUNKVLANS;
		break;
	case CMD_VLAN_REMOVE:
		parse_vlan_err = parse_vlan_list(argv[argc - 1], bmp);
		swcfgr.cmd = SWCFG_DELTRUNKVLANS;
		break;
	}

	if (parse_vlan_err) {
		EX_STATUS_REASON(ctx, "Bad VLAN list");
		return CLI_EX_REJECTED;
	}

	swcfgr.ifindex = uc->ifindex;
	swcfgr.ext.bmp = bmp;

	SW_SOCK_OPEN(ctx, sock_fd);
	ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd);

	return CLI_EX_OK;
}

int cmd_nomode(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcfgreq swcfgr;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	int sock_fd;

	SW_SOCK_OPEN(ctx, sock_fd);

	swcfgr.cmd = SWCFG_SETTRUNK;
	swcfgr.ifindex = uc->ifindex;
	swcfgr.ext.trunk = 0;
	ioctl(sock_fd, SIOCSWCFG, &swcfgr);

	swcfgr.cmd = SWCFG_SETACCESS;
	swcfgr.ext.access = 0;
	ioctl(sock_fd, SIOCSWCFG, &swcfgr);

	SW_SOCK_CLOSE(ctx, sock_fd);

	return CLI_EX_OK;
}

int cmd_swport_on(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

int cmd_acc_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcfgreq swcfgr;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	int sock_fd;

	SW_SOCK_OPEN(ctx, sock_fd);

	swcfgr.cmd = SWCFG_SETPORTVLAN;
	swcfgr.ifindex = uc->ifindex;
	swcfgr.vlan = 1;
	if (strcmp(nodev[0]->name, "no"))
		swcfgr.vlan = atoi(argv[3]);
	ioctl(sock_fd, SIOCSWCFG, &swcfgr);

	SW_SOCK_CLOSE(ctx, sock_fd);

	return CLI_EX_OK;
}

int count_mask_bits(uint32_t n_mask)
{
	uint32_t h_mask = ntohl(n_mask);
	int ret = 0;

	while (h_mask & 0x80000000) {
		ret++;
		h_mask <<= 1;
	}

	return h_mask ? -1 : ret;
}

int cmd_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int ret = CLI_EX_OK;
	int cmd = RTM_NEWADDR;
	int sock_fd = -1, secondary = 0;
	int has_primary, status, addr_cnt = 0;
	struct ifreq ifr;
    struct swcli_context *uc = SWCLI_CTX(ctx);
	struct in_addr addr, mask;
	int mask_len;
	LIST_HEAD(addrl);
	struct if_addr *if_addr;

	if (!strcmp(nodev[0]->name, "no")) {
		cmd = RTM_DELADDR;
		SHIFT_ARG(argc, argv, nodev);
	}

	/* skip next 2 args (ip address) */
	assert(argc > 2);
	SHIFT_ARG(argc, argv, nodev, 2);
	assert(argc >= 2);

	/* semantically validate mask first; note that both address and mask 
	 * are syntactically validated by the cli */
	status = inet_aton(argv[0], &addr);
	assert(status);
	status = inet_aton(argv[1], &mask);
	assert(status);
	if ((mask_len = count_mask_bits(mask.s_addr)) < 0) {
		EX_STATUS_REASON(ctx, "Bad mask 0x%X for address %s", ntohl(mask.s_addr), inet_ntoa(addr));
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}
	
	if (!(addr.s_addr & ~mask.s_addr) || (addr.s_addr & ~mask.s_addr) == ~mask.s_addr) {
		EX_STATUS_REASON(ctx, "Bad mask /%d for address %s", mask_len, inet_ntoa(addr));
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}

	if (argc > 2 && !strcmp(nodev[2]->name, "secondary"))
		secondary = 1;
	
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock_fd != -1);

	/* determine if we have a primary address
	 * using netdevice ioctl() (man 7 netdevice) is a bit lame since
	 * it requires device name (not index) and we must convert back
	 * index to name */

	ifr.ifr_ifindex = uc->ifindex;
	status = ioctl(sock_fd, SIOCGIFNAME, &ifr);
	assert(!status);
	has_primary = !ioctl(sock_fd, SIOCGIFADDR, &ifr);

	if (secondary && has_primary && ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr == addr.s_addr) {
		EX_STATUS_REASON(ctx, "Secondary can't be same as primary");
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}

	if (if_get_addr(0, AF_INET, &addrl, NULL))
		goto out_cleanup;

	/* test if address overlaps with other interface */
	list_for_each_entry(if_addr, &addrl, lh) {
		if (if_addr->ifindex == uc->ifindex) {
			addr_cnt++;
			continue;
		}
		if (!ip_addr_overlap(addr, mask_len, if_addr->inet, if_addr->prefixlen))
			continue;
		/* determine overlapping interface name */
		ifr.ifr_ifindex = if_addr->ifindex;
		status = ioctl(sock_fd, SIOCGIFNAME, &ifr);
		assert(!status);
		/* strip host part for displaying error msg */
		addr.s_addr &= mask.s_addr;
		EX_STATUS_REASON(ctx, "%s overlaps with %s", inet_ntoa(addr), ifr.ifr_name);
		ret = CLI_EX_WARNING;

		goto out_cleanup;
	}

	/* implement address deletion first, as it's less complicated */
	if (cmd == RTM_DELADDR) {
		/* if address count is at least 2, then we have a primary
		 * address AND at least 1 secodary address */
		if (!secondary && addr_cnt >= 2) {
			EX_STATUS_REASON(ctx, "Must delete secondary before deleting primary");
			ret = CLI_EX_WARNING;
			goto out_cleanup;
		}
		list_for_each_entry(if_addr, &addrl, lh) {
			if (if_addr->ifindex != uc->ifindex)
				continue;
			if (!secondary && (if_addr->inet.s_addr != addr.s_addr || if_addr->prefixlen != mask_len)) {
				EX_STATUS_REASON(ctx, "Invalid address");
				ret = CLI_EX_WARNING;
				goto out_cleanup;
			}
			if (if_addr->inet.s_addr != addr.s_addr)
				continue;
			if (if_addr->prefixlen != mask_len) {
				EX_STATUS_REASON(ctx, "Invalid Mask");
				ret = CLI_EX_WARNING;
				goto out_cleanup;
			}
			if_change_addr(cmd, uc->ifindex, addr, mask_len, secondary, NULL);
		}
		goto out_cleanup;
	}

	/* if control reaches this point, cmd is RTM_NEWADDR */
	if (secondary && !has_primary)
		goto out_cleanup;

	/* unless we are overwriting the primary ip, we must first delete
	 * a similar address using netlink; the logic for "secondary" below
	 * is this: if secondary is true (1), then we already checked (a few
	 * lines of code before) that (user-supplied) argument address is
	 * different from primary address, so it's ok to walk the list and
	 * delete any matching address */
	if (secondary || (has_primary && addr.s_addr != ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr))
		list_for_each_entry(if_addr, &addrl, lh) {
			if (if_addr->ifindex != uc->ifindex)
				continue;
			if (if_addr->inet.s_addr != addr.s_addr)
				continue;
			if_change_addr(RTM_DELADDR, if_addr->ifindex, if_addr->inet, if_addr->prefixlen, 1, NULL);
			break;
		}

	if (secondary) {
		if_change_addr(cmd, uc->ifindex, addr, mask_len, secondary, NULL);
		goto out_cleanup;
	}

	/* use old school ioctl() to change primary ip */

	ifr.ifr_addr.sa_family = AF_INET;
	((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr = addr;
	if (ioctl(sock_fd, SIOCSIFADDR, &ifr)) {
		EX_STATUS_REASON(ctx, "ioctl (SIOCSIFADDR) failed: %s", strerror(errno));
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}

	ifr.ifr_netmask.sa_family = AF_INET;
	((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr = mask;
	if (ioctl(sock_fd, SIOCSIFNETMASK, &ifr)) {
		EX_STATUS_REASON(ctx, "ioctl (SIOCSIFNETMASK) failed: %s", strerror(errno));
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}

	ifr.ifr_broadaddr.sa_family = AF_INET;
	((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr =
		addr.s_addr | ~mask.s_addr;
	if (ioctl(sock_fd, SIOCSIFBRDADDR, &ifr)) {
		EX_STATUS_REASON(ctx, "ioctl (SIOCSIFBRDADDR) failed: %s", strerror(errno));
		ret = CLI_EX_WARNING;
		goto out_cleanup;
	}
	
out_cleanup:
	if (sock_fd != -1)
		close(sock_fd);
	list_free(&addrl, struct if_addr, lh);
	return ret;
}

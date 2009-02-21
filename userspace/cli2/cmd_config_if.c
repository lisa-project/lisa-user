#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include "swsock.h"
#include "netlink.h"

#include "cli.h"
#include "swcli_common.h"
#include "cmd_config_if.h"

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')
// FIXME move is_digit() to .h

static int parse_vlan_list(char *list, unsigned char *bmp) {
	int state = 0;
	int min, max;
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

int cmd_cdp_if_enable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

int cmd_if_desc(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;
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

int cmd_du_auto(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_du_full(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_du_half(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_cdp_if_disable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_sp_auto(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_swport_off(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_noacc_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

int cmd_trunk_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{ 
	int parse_vlan_err = 0, i, sock_fd;
	struct swcfgreq swcfgr;
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;
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

int cmd_nomode(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_sp_10(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_sp_100(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_swport_on(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_acc_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

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

void ip_netlink(int cmd)
{
}

int cmd_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int cmd = RTM_NEWADDR;
	int sock_fd, secondary = 0;
	int has_primary, status;
	struct ifreq ifr;
    struct rlshell_context *rlctx = (void *)ctx;
    struct swcli_context *uc = (void*)rlctx->uc;
	struct in_addr addr, mask;
	int mask_len;

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
		printf("Bad mask 0x%X for address %s\n", ntohl(mask.s_addr), inet_ntoa(addr));
		//FIXME output
		return CLI_EX_OK;
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
		printf("Secondary can't be same as primary\n"); //FIXME output
		close(sock_fd);
		return CLI_EX_OK;
	}

	switch (cmd) {
	case RTM_NEWADDR:
		if (secondary && !has_primary)
			break;
		if (secondary) {
			ip_netlink(cmd);
			break;
		}

		/* use old school ioctl() to change primary ip */

		ifr.ifr_addr.sa_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr = addr;
		if (ioctl(sock_fd, SIOCSIFADDR, &ifr)) {
			perror("SIOCSIFADDR"); // FIXME output
			break;
		}

		ifr.ifr_netmask.sa_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr = mask;
		if (ioctl(sock_fd, SIOCSIFNETMASK, &ifr)) {
			perror("SIOCSIFNETMASK"); // FIXME output
			break;
		}

		ifr.ifr_broadaddr.sa_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr =
			addr.s_addr | ~mask.s_addr;
		if (ioctl(sock_fd, SIOCSIFBRDADDR, &ifr)) {
			perror("SIOCSIFBRDADDR"); // FIXME output
			break;
		}
		break;
	case RTM_DELADDR:
		if (!secondary && has_primary) { // FIXME FIXME FIXME aici tb sa testez daca are cel putin 2 adrese => sigur are cel putin una secondary
			printf("Must delete secondary before deleting primary\n"); //FIXME output
			break;
		}
		ip_netlink(cmd);
		break;
	}
	
	close(sock_fd);
	return CLI_EX_OK;
}

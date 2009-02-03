#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include "swsock.h"

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
int cmd_setethdesc(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_du_auto(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_du_full(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_du_half(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_cdp_if_disable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_noethdesc(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
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

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	close(sock_fd);

	return CLI_EX_OK;
}

int cmd_nomode(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_sp_10(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_sp_100(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_swport_on(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_acc_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

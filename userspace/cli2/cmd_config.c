#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

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
#include "menu_interface.h"

int cmd_cdp_version(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_setenpw(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_setenpwlev(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_end(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_hostname(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

extern char curr_if_name[]; //FIXME move to .h

static int use_if_ether(struct cli_context *ctx, char *name, int index, int switchport)
{
	int status, sock_fd, ioctl_errno;
	struct swcfgreq swcfgr;
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	/* make sure we have the interface index */
	do {
		struct ifreq ifr;

		if (index)
			break;

		strncpy(ifr.ifr_name, name, IFNAMSIZ);

		if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr)) {
			index = ifr.ifr_ifindex;
			break;
		}

		EX_STATUS_REASON(ctx, "interface %s does not exist", name);
		close(sock_fd);
		return CLI_EX_REJECTED;
	} while (0);

	swcfgr.cmd = SWCFG_ADDIF;
	swcfgr.ifindex = index;
	swcfgr.ext.switchport = switchport;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd); /* this can overwrite ioctl errno */

	if (status) {
		switch (ioctl_errno) {
		case ENODEV:
			EX_STATUS_REASON(ctx, "interface %s does not exist", name);
			return CLI_EX_REJECTED;
		case EINVAL:
			EX_STATUS_REASON(ctx, "interface %s is a VIF", name);
			return CLI_EX_REJECTED;
		case EBUSY:
			break;
		default:
			EX_STATUS_REASON_IOCTL(ctx, ioctl_errno);
			return CLI_EX_REJECTED;
		}
	} else {
		/* Enable CDP on this interface */
		// FIXME cdp_adm_query(CDP_IF_ENABLE, ioctl_arg.if_name);
	}

	uc->ifindex = index;

	// FIXME change menu root
	return CLI_EX_OK;
}

int use_if_vlan(struct cli_context *ctx, int vlan, int index)
{
	int status, sock_fd, ioctl_errno;
	struct swcfgreq swcfgr;
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	swcfgr.cmd = SWCFG_ADDVIF;
	swcfgr.vlan = vlan;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd);

	if (status && ioctl_errno != EEXIST) {
		EX_STATUS_REASON_IOCTL(ctx, ioctl_errno);
		return CLI_EX_REJECTED;
	}

	uc->ifindex = swcfgr.ifindex;

	// FIXME change menu root
	return CLI_EX_OK;
}

int cmd_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, sock_fd, ioctl_errno;
	struct ifreq ifr;
	struct swcfgreq swcfgr;

	if (!strcmp(nodev[1]->name, "Ethernet")) {
		int n = atoi(argv[2]);
		char name[IFNAMSIZ];

		/* convert forth and back to int to avoid 0-left-padded numbers */
		status = snprintf(name, IFNAMSIZ, "eth%d", n);
		assert(status < IFNAMSIZ);
		return use_if_ether(ctx, name, 0, 1);
	}

	if (!strcmp(nodev[1]->name, "vlan")) {
		int n = atoi(argv[2]);
		return use_if_vlan(ctx, n, 0);
	}

	if (strcmp(nodev[1]->name, "netdev")) {
		ctx->ex_status.reason = NULL;
		return CLI_EX_REJECTED;
	}

	/* try to guess what netdev name means */

	if (strlen(argv[2]) >= IFNAMSIZ) {
		EX_STATUS_REASON(ctx, "invalid interface name");
		return CLI_EX_REJECTED;
	}

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	/* first test if the interface already exists; SIOCGIFINDEX works
	 * on any socket type (see man (7) netdevice for details) */
	strncpy(ifr.ifr_name, argv[2], IFNAMSIZ);
	do {
		if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr))
			break;
		close(sock_fd);

		/* test for VIF addition */
		status = parse_vlan(argv[2]);
		if (status >= 0)
			return use_if_vlan(ctx, status, 0);

		/* for now no other type of interfaces can be added */
		EX_STATUS_REASON(ctx, "interface %s does not exist", argv[2]);
		return CLI_EX_REJECTED;
	} while (0);

	/* ask switch kernel module what it knows about this interface */
	swcfgr.cmd = SWCFG_GETIFTYPE;
	swcfgr.ifindex = ifr.ifr_ifindex;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd);

	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, ioctl_errno);
		return CLI_EX_REJECTED;
	}

	if (swcfgr.ext.switchport == SW_IF_VIF)
		return use_if_vlan(ctx, swcfgr.vlan, ifr.ifr_ifindex);

	return use_if_ether(ctx, argv[2], ifr.ifr_ifindex,
			swcfgr.ext.switchport != SW_IF_ROUTED);
}

int cmd_linevty(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_set_aging(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_macstatic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_cdp_v2(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_noensecret(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_noensecret_lev(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_nohostname(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_int_eth(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_int_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_no_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_set_noaging(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_novlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

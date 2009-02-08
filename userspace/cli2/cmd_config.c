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

extern struct menu_node menu_main;
extern struct menu_node config_main;
extern struct menu_node config_if_main;
extern struct menu_node config_vlan_main;

int cmd_end(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	ctx->root = &menu_main;
	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	return CLI_EX_OK;
}

int cmd_exit(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	ctx->root =& config_main;
	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	return CLI_EX_OK;
}

int cmd_hostname(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

static int use_if_ether(struct cli_context *ctx, char *name, int index, int switchport)
{
	int status, sock_fd, ioctl_errno;
	struct swcfgreq swcfgr;
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	if (!index)
		index = if_get_index(name, sock_fd);

	if (!index) {
		EX_STATUS_REASON(ctx, "interface %s does not exist", name);
		close(sock_fd);
		return CLI_EX_REJECTED;
	}

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

	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	ctx->node_filter |= IFF_SWITCHED;
	ctx->root = &config_if_main;
	uc->ifindex = index;
	uc->vlan = -1;

	return CLI_EX_OK;
}

static int use_if_vlan(struct cli_context *ctx, int vlan, int index)
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

	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	ctx->node_filter |= IFF_VIF;
	ctx->root = &config_if_main;
	uc->ifindex = swcfgr.ifindex;
	uc->vlan = vlan;

	return CLI_EX_OK;
}

static int remove_if_ether(struct cli_context *ctx, char *name, int index, int switchport)
{
	int sock_fd;
	struct swcfgreq swcfgr;

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	if (!index)
		index = if_get_index(name, sock_fd);

	if (!index) {
		close(sock_fd);
		return CLI_EX_OK;
	}

	swcfgr.cmd = SWCFG_DELIF;
	swcfgr.ifindex = index;

	/* Disable CDP on this interface */
	//FIXME cdp_adm_query(CDP_IF_DISABLE, ioctl_arg.ifindex);
	
	/* FIXME nu avem un race aici? codul de kernel pentru socketzi
	 * are nevoie ca device-ul sa fie port in switch => nu poate fi
	 * scos portul pana nu se inchid toti socketzii; aici doar trimit
	 * comanda prin ipc si dureaza pana cdpd inchide socketul.
	 */

	ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	close(sock_fd);

	return CLI_EX_OK;
}

static int remove_if_vlan(struct cli_context *ctx, int vlan, int index)
{
	int status, sock_fd, ioctl_errno;
	struct swcfgreq swcfgr;

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	swcfgr.cmd = SWCFG_DELVIF;
	swcfgr.vlan = vlan;
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd);

	if (status && ioctl_errno != ENOENT) {
		EX_STATUS_REASON_IOCTL(ctx, ioctl_errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

static int cmd_no_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, n, sock_fd, ioctl_errno;
	struct ifreq ifr;
	struct swcfgreq swcfgr;

	status = if_parse_args(argv + 1, nodev + 1, ifr.ifr_name, &n);

	switch (status) {
	case IF_T_ETHERNET:
		return remove_if_ether(ctx, ifr.ifr_name, 0, 1);
	case IF_T_VLAN:
		return remove_if_vlan(ctx, n, 0);
	}

	if (status == IF_T_ERROR && n == -1) {
		EX_STATUS_REASON(ctx, "invalid interface name");
		return CLI_EX_REJECTED;
	}

	if (status != IF_T_NETDEV) {
		ctx->ex_status.reason = NULL;
		return CLI_EX_REJECTED;
	}

	/* try to guess what netdev name means */

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	if (ioctl(sock_fd, SIOCGIFINDEX, &ifr)) {
		close(sock_fd);
		return CLI_EX_OK;
	}

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
		return remove_if_vlan(ctx, swcfgr.vlan, ifr.ifr_ifindex);

	return remove_if_ether(ctx, ifr.ifr_name, ifr.ifr_ifindex,
			swcfgr.ext.switchport != SW_IF_ROUTED);
}

int cmd_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, n, sock_fd, ioctl_errno;
	struct ifreq ifr;
	struct swcfgreq swcfgr;

	if (!strcmp(nodev[0]->name, "no"))
		return cmd_no_int_any(ctx, argc - 1, argv + 1, nodev + 1);

	status = if_parse_args(argv + 1, nodev + 1, ifr.ifr_name, &n);

	switch (status) {
	case IF_T_ETHERNET:
		return use_if_ether(ctx, ifr.ifr_name, 0, 1);
	case IF_T_VLAN:
		return use_if_vlan(ctx, n, 0);
	}

	if (status == IF_T_ERROR && n == -1) {
		EX_STATUS_REASON(ctx, "invalid interface name");
		return CLI_EX_REJECTED;
	}

	if (status != IF_T_NETDEV) {
		ctx->ex_status.reason = NULL;
		return CLI_EX_REJECTED;
	}

	/* try to guess what netdev name means */

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	/* first test if the interface already exists; SIOCGIFINDEX works
	 * on any socket type (see man (7) netdevice for details) */
	do {
		if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr))
			break;
		close(sock_fd);

		/* test for VIF addition */
		status = if_parse_vlan(ifr.ifr_name);
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

	return use_if_ether(ctx, ifr.ifr_name, ifr.ifr_ifindex,
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
int cmd_set_noaging(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }
int cmd_novlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) { return 0; }

int cmd_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcfgreq swcfgr;
	int status, sock_fd, ioctl_errno;
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;

	swcfgr.vlan = atoi(argv[argc - 1]);
	if (strcmp(nodev[0]->name, "no")) {
		/* vlan is added */
		swcfgr.cmd = SWCFG_ADDVLAN;
		default_vlan_name(swcfgr.ext.vlan_desc, swcfgr.vlan);
	} else {
		/* vlan is removed */
		swcfgr.cmd = SWCFG_DELVLAN;
	}

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd); /* this can overwrite ioctl errno */

	if (status == -1 && ioctl_errno == EACCES)
		printf("Default VLAN %d may not be deleted.\n", swcfgr.vlan); //FIXME output

	if (swcfgr.cmd == SWCFG_ADDVLAN && (!status || ioctl_errno == EEXIST)) {
		ctx->root = &config_vlan_main;
		uc->vlan = swcfgr.vlan;
	}
	
	return CLI_EX_OK;
}

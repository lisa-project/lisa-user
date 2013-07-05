#include "swcli.h"
#include "config.h"

#include <regex.h>

extern struct menu_node menu_main;
extern struct menu_node config_main;
extern struct menu_node config_if_main;
extern struct menu_node config_vlan_main;
extern struct menu_node config_line_main;

static char hostname_default[] = "Switch\0";

int cmd_rstp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rstp_configuration rstp;
	int enabled = 1;

	if (!strcmp(nodev[0]->name, "no"))
		enabled = 0;
	switch_get_rstp(&rstp);
	rstp.enabled = enabled;
	switch_set_rstp(&rstp);

	return CLI_EX_OK;
}

int cmd_cdp_v2(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	int version = 2;

	if (!strcmp(nodev[0]->name, "no"))
		version = 1;

	switch_get_cdp(&cdp);
	if (cdp.enabled) {
		cdp.version = version;
		switch_set_cdp(&cdp);
	}

	return CLI_EX_OK;
}

int cmd_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;

	switch_get_cdp(&cdp);
	if (cdp.enabled) {
		cdp.holdtime = atoi(argv[2]);
		switch_set_cdp(&cdp);
	}

	return CLI_EX_OK;
}

int cmd_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;

	switch_get_cdp(&cdp);
	if (cdp.enabled) {
		cdp.timer = atoi(argv[2]);
		switch_set_cdp(&cdp);
	}

	return CLI_EX_OK;
}

int cmd_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	int enabled = 1;

	if (!strcmp(nodev[0]->name, "no"))
		enabled = 0;
	switch_get_cdp(&cdp);
	cdp.enabled = enabled;
	switch_set_cdp(&cdp);

	return CLI_EX_OK;
}

static int _cmd_setenpw(struct cli_context *ctx, int level, char *passwd)
{
	int err;
	if ((err = switch_set_passwd(SHARED_AUTH_ENABLE, level, passwd))) {
		EX_STATUS_REASON(ctx, "%s", strerror(err));
		return CLI_EX_REJECTED;
	}
	return CLI_EX_OK;
}

int cmd_setenpw(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	char *clear_pw = argv[argc - 1],
		 *encrypted_pw;
	const char salt_base[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
	char salt[] = "$1$....$\0";
	int i, level = SW_MAX_ENABLE;

	/* Clear text password; need to crypt() */
	srandom(time(NULL) & 0xffffffffL);
	for (i = 3; i < 7; i++)
		salt[i] = salt_base[random() % 64];
	encrypted_pw = crypt(clear_pw, salt);

	if (argc > 4)
		level = atoi(argv[3]);

	return _cmd_setenpw(ctx, level, encrypted_pw);
}

int cmd_setenpw_encrypted(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	char *encrypted_pw = argv[argc - 1];
	int status, level = SW_MAX_ENABLE;
	regex_t regex;
	regmatch_t result;

	/* Encrypted password; need to check syntax */
	status = regcomp(&regex,  "^\\$1\\$[a-zA-Z0-9\\./]{4}\\$[a-zA-Z0-9\\./]{22}$", REG_EXTENDED);
	assert(!status);
	if(regexec(&regex, *argv, 1, &result, 0)) {
		out = ctx->out_open(ctx, 0);
		fputs("ERROR: The secret you entered is not a valid encrypted secret.\n"
				"To enter an UNENCRYPTED secret, do not specify type 5 encryption.\n"
				"When you properly enter an UNENCRYPTED secret, it will be encrypted.\n\n",
				out);
		return CLI_EX_REJECTED;
	}

	if (argc > 4)
		level = atoi(argv[3]);

	return _cmd_setenpw(ctx, level, encrypted_pw);
}

int cmd_noensecret(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int level = SW_MAX_ENABLE;

	if (argc == 5)
		level = atoi(argv[4]);

	return _cmd_setenpw(ctx, level, (char *)"\0");
}

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

int cmd_hostname(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	char *hostname = argv[1];

	if (!strcmp(nodev[0]->name, "no"))
		hostname = hostname_default;

	if (sethostname(hostname, strlen(hostname))) {
		EX_STATUS_REASON(ctx, "%s", strerror(errno));
		return CLI_EX_REJECTED;
	}
	return CLI_EX_OK;
}

static int use_if_ether(struct cli_context *ctx, char *name, int index, int switchport)
{
	int status, sock_fd, ioctl_errno;
	struct swcli_context *uc = SWCLI_CTX(ctx);
	struct cdp_session *cdp;
	FILE *out;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (!index)
		index = if_get_index(name, sock_fd);

	if (!index) {
		EX_STATUS_REASON(ctx, "interface %s does not exist", name);
		SW_SOCK_CLOSE(ctx, sock_fd);
		return CLI_EX_REJECTED;
	}

	status = if_add(DEFAULT_SW, name, switchport);

	ioctl_errno = errno;

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
		/* Try to enable CDP on this interface */
		do {
			if (!CDP_SESSION_OPEN(ctx, cdp)) {
				out = ctx->out_open(ctx, 0);
				fprintf(out, "WARNING: failed to enable cdp on interface %s : %s\n",
						name, strerror(errno));
				break;
			}
			cdp_set_interface(cdp, index, 1);
			CDP_SESSION_CLOSE(ctx, cdp);
		} while (0);
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
	int status;
	int ifindex;
	struct swcli_context *uc = SWCLI_CTX(ctx);

	status = sw_ops->vif_add(sw_ops, vlan, &ifindex);
	if (status && errno != EEXIST) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	ctx->node_filter &= PRIV_FILTER(PRIV_MAX);
	ctx->node_filter |= IFF_VIF;
	ctx->root = &config_if_main;
	uc->ifindex = ifindex;
	uc->vlan = vlan;

	return CLI_EX_OK;
}

static int remove_if_ether(struct cli_context *ctx, char *name, int index, int switchport)
{
	int sock_fd;
	int status;
	struct cdp_session *cdp;
	FILE *out;

	SW_SOCK_OPEN(ctx, sock_fd);

	if (!index)
		index = if_get_index(name, sock_fd);

	if (!index) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return CLI_EX_OK;
	}

	/* Try to disable CDP on this interface */
	do {
		if (!CDP_SESSION_OPEN(ctx, cdp)) {
			out = ctx->out_open(ctx, 0);
			fprintf(out, "WARNING: failed to disable cdp on interface %s: %s\n",
					name, strerror(errno));
			break;
		}
		cdp_set_interface(cdp, index, 0);
		CDP_SESSION_CLOSE(ctx, cdp);
	} while (0);

	SW_SOCK_CLOSE(ctx, sock_fd);

	//status = sw_ops->if_remove(sw_ops, index);
	status = if_remove(DEFAULT_SW, name);
	if (status) {
		EX_STATUS_REASON(ctx, "if_remove failed");
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

static int remove_if_vlan(struct cli_context *ctx, int vlan, int index)
{
	int status;

	status = sw_ops->vif_del(sw_ops, vlan);
	if (status && errno != ENOENT) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

static int cmd_no_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, n, sock_fd;
	struct ifreq ifr;
	int iftype, ifvlan = 0;

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

	SW_SOCK_OPEN(ctx, sock_fd);

	if (ioctl(sock_fd, SIOCGIFINDEX, &ifr)) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return CLI_EX_OK;
	}
	SW_SOCK_CLOSE(ctx, sock_fd);

	/* ask switch kernel module what it knows about this interface */
	//status = (sw_ops->if_get_type)(sw_ops, ifr.ifr_ifindex, &iftype, &ifvlan);
	status = if_get_type_api(DEFAULT_SW, ifr.ifr_name, &iftype, &ifvlan);

	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	if (iftype == IF_TYPE_VIF)
		return remove_if_vlan(ctx, ifvlan, ifr.ifr_ifindex);

	return remove_if_ether(ctx, ifr.ifr_name, ifr.ifr_ifindex,
			iftype != IF_TYPE_ROUTED);
}

int cmd_int_any(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, n, sock_fd;
	struct ifreq ifr;
	int iftype, ifvlan;

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

	SW_SOCK_OPEN(ctx, sock_fd);

	/* first test if the interface already exists; SIOCGIFINDEX works
	 * on any socket type (see man (7) netdevice for details) */
	do {
		if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr))
			break;
		SW_SOCK_CLOSE(ctx, sock_fd);

		/* test for VIF addition */
		status = if_parse_vlan(ifr.ifr_name);
		if (status >= 0)
			return use_if_vlan(ctx, status, 0);

		/* for now no other type of interfaces can be added */
		EX_STATUS_REASON(ctx, "interface %s does not exist", argv[2]);
		return CLI_EX_REJECTED;
	} while (0);

	SW_SOCK_CLOSE(ctx, sock_fd);

	/* ask switch kernel module what it knows about this interface */
//	status = (sw_ops->if_get_type)(sw_ops, ifr.ifr_ifindex, &iftype, &ifvlan);
	status = if_get_type_api(DEFAULT_SW, ifr.ifr_name, &iftype, &ifvlan);

	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	if (iftype == IF_TYPE_VIF)
		return use_if_vlan(ctx, ifvlan, ifr.ifr_ifindex);

	return use_if_ether(ctx, ifr.ifr_name, ifr.ifr_ifindex,
			iftype != IF_TYPE_ROUTED);
}

int cmd_linevty(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *ctx = (void *)__ctx;
	int min, max;

	min = atoi(argv[argc - 2]);
	max = atoi(argv[argc - 1]);
	if (min > max) {
		EX_STATUS_REASON(ctx, "first line number %d > last line number %d",
				min, max);
		return CLI_EX_REJECTED;
	}
	ctx->cc.root = &config_line_main;
	ctx->enable_ctrl_z = 1;
	return CLI_EX_OK;
}

int cmd_set_aging(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, nsec = SW_DEFAULT_AGE_TIME;

	if (strcmp(nodev[0]->name, "no"))
		nsec = atoi(argv[2]);

	status = sw_ops->set_age_time(sw_ops, nsec);
	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

int cmd_macstatic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int sock_fd, status;
	int ifindex, vlan;
	int delmac_flag = 0;
	unsigned char mac[ETH_ALEN];

	if (!strcmp(nodev[0]->name, "no")) {
		delmac_flag = 1;
		SHIFT_ARG(argc, argv, nodev);
	}

	if ((status = parse_mac(argv[2], mac))) {
		EX_STATUS_REASON(ctx, "Error parsing mac address %s: %s", argv[2], strerror(status));
		return CLI_EX_REJECTED;
	}

	SW_SOCK_OPEN(ctx, sock_fd);

	vlan = atoi(argv[4]);
	SHIFT_ARG(argc, argv, nodev, 6);
	if_args_to_ifindex(ctx, argv, nodev, sock_fd, ifindex, status);
	if (delmac_flag)
		status = sw_ops->vlan_del_mac_static(sw_ops, ifindex, vlan, mac);
	else
		status = sw_ops->vlan_set_mac_static(sw_ops, ifindex, vlan, mac);
	SW_SOCK_CLOSE(ctx, sock_fd);

	if (status) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

int cmd_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, vlan, ret = CLI_EX_OK, added, cmd_errno;
	struct swcli_context *uc = SWCLI_CTX(ctx);

	vlan = atoi(argv[argc - 1]);
	if (strcmp(nodev[0]->name, "no")) {
		/* vlan is added */
		status = sw_ops->vlan_add(sw_ops, vlan);
		cmd_errno = errno;
		if (status == 0) {
			char desc[SW_MAX_VLAN_NAME];
			__default_vlan_name(desc, vlan);
			status = switch_set_vlan_desc(vlan, desc);
			if (status) {
				EX_STATUS_PERROR(ctx, "addvlan failed");
				sw_ops->vlan_del(sw_ops, vlan);
				ret = CLI_EX_REJECTED;
				goto out;
			}
		}
		added = 1;
	} else {
		/* vlan is removed */
		status = sw_ops->vlan_del(sw_ops, vlan);
		cmd_errno = errno;
		if (status == 0)
			status = switch_del_vlan_desc(vlan);
		added = 0;
	}

	if (status && cmd_errno == EACCES) {
		EX_STATUS_REASON(ctx, "Default VLAN %d may not be deleted.\n",
				vlan);
		ret = CLI_EX_REJECTED;
	}

	if (added && (!status || cmd_errno == EEXIST)) {
		ctx->root = &config_vlan_main;
		uc->vlan = vlan;
	}
out:
	return ret;
}

int cmd_add_mrouter(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int sock_fd, iftype, status;
	int mrouter = 1, vlan, ifindex;

	if (!strcmp(nodev[0]->name, "no")) {
		mrouter = 0;
		SHIFT_ARG(argc, argv, nodev);
	}

	SHIFT_ARG(argc, argv, nodev, 4);
	vlan = atoi(argv[0]);

	SHIFT_ARG(argc, argv, nodev, 3);
	SW_SOCK_OPEN(ctx, sock_fd);
	if_args_to_ifindex(ctx, argv, nodev, sock_fd, ifindex, iftype);
	SW_SOCK_CLOSE(ctx, sock_fd);
	SHIFT_ARG(argc, argv, nodev);

	status = sw_ops->mrouter_set(sw_ops, vlan, ifindex, mrouter);

	if (status == -1) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

int cmd_ip_igmp_snooping(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	/* #<no> ip igmp snooping <vlan <number>>*/
	int status;
	int vlan = 0, snooping = 1;

	if (!strcmp(nodev[0]->name, "no")) {
		snooping = 0;
		SHIFT_ARG(argc, argv, nodev);
	}

	if(argc >= 5){
		vlan = atoi(argv[4]);
	}

	status = sw_ops->igmp_set(sw_ops, vlan, snooping);

	if (status == -1) {
		EX_STATUS_REASON_IOCTL(ctx, errno);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

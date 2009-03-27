#include "swcli.h"
#include "config_vlan.h"

int cmd_namevlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcli_context *uc = SWCLI_CTX(ctx);
	struct swcfgreq swcfgr;
	int status, sock_fd, ioctl_errno;
	int u_rename = 0;

	assert(argc);

	swcfgr.cmd = SWCFG_RENAMEVLAN;
	swcfgr.vlan = uc->vlan;
	if (strcmp(nodev[0]->name, "no")) {
		/* vlan is renamed by user */
		assert(argc >= 2);
		swcfgr.ext.vlan_desc = argv[1];
		u_rename = 1;
	} else {
		/* vlan name is set to default */
		default_vlan_name(swcfgr.ext.vlan_desc, uc->vlan);
	}

	SW_SOCK_OPEN(ctx, sock_fd);
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (u_rename && status == -1 && ioctl_errno == EPERM)
		printf("Default VLAN %d may not have its name changed.\n", uc->vlan); // FIXME output

	return CLI_EX_OK;
}

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

int cmd_namevlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;
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

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	close(sock_fd); /* this can overwrite ioctl errno */

	if (u_rename && status == -1 && ioctl_errno == EPERM)
		printf("Default VLAN %d may not have its name changed.\n", uc->vlan); // FIXME output

	return CLI_EX_OK;
}

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

char *swcli_prompt(struct rlshell_context *ctx) {
	char hostname[HOST_NAME_MAX + 1];
	size_t buf_size = HOST_NAME_MAX + MENU_NAME_MAX + 3;
	char *buf = malloc(buf_size);
	char prompt = ctx->cc.node_filter & PRIV(2) ? '#' : '>';

	if (buf == NULL)
		return buf;

	gethostname(hostname, HOST_NAME_MAX);
	hostname[HOST_NAME_MAX] = '\0';

	if (ctx->cc.root->name == NULL)
		snprintf(buf, buf_size, "%s%c", hostname, prompt);
	else
		snprintf(buf, buf_size, "%s(%s)%c", hostname,
				ctx->cc.root->name, prompt);

	return buf;
}

int cmd_ioctl_simple(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *rlctx = (void *)ctx;
	struct swcli_context *uc = (void*)rlctx->uc;
	struct swcfgreq swcfgr, **rp;
	int sock_fd;

	assert(argc);

	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock_fd != -1);

	for (rp = nodev[argc - 1]->priv; *rp; rp++) {
		swcfgr = **rp;
		swcfgr.ifindex = uc->ifindex;
		swcfgr.vlan = uc->vlan;
		ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	}

	close(sock_fd);

	return CLI_EX_OK;
}

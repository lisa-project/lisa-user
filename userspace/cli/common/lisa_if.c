#include "lisa.h"

struct lisa_context lisa_ctx;

int if_add(struct switch_operations *sw_ops, int ifindex, int switchport)
{
	FILE *out;
	int status, sock_fd;
	struct cli_context ctx;
	struct swcfgreq swcfgr;
	struct lisa_context *lctx;
	struct cdp_session *cdp;
	struct swcli_context *uc;

	/* Get the lisa context */
	lctx = (struct lisa_context*) sw_ops;
	ctx = lctx->ctx;
	uc = SWCLI_CTX(ctx);

	swcfgr.cmd = SWCFG_ADDIF;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.switchport = switchport;

	SW_SOCK_OPEN(ctx, sock_fd);
	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	return status;
}

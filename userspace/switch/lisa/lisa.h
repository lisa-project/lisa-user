#ifndef _LISA_H
#define _LISA_H

#include <unistd.h>

#include <linux/sockios.h>

#include "sw_api.h"
#include "swsock.h"

#define SWLiSA_CTX(sw_ops) ((struct lisa_context *)(sw_ops))

#define SW_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (__ctx->sock_fd != -1) {\
		__sock_fd = __ctx->sock_fd;\
		break;\
	}\
	__sock_fd = socket(PF_SWITCH, SOCK_RAW, 0); \
	if (__sock_fd == -1) \
		return -1; \
} while(0)

#define SW_SOCK_CLOSE(__ctx, __sock_fd) do {\
	if (__sock_fd != __ctx->sock_fd)\
		close(__sock_fd);\
} while (0)


struct lisa_context {

	struct switch_operations sw_ops;

	/* Socket descriptor used for communication with LiSA module. */
	int sock_fd;
};

extern struct lisa_context lisa_ctx;
#endif

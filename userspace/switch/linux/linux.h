#ifndef _LINUX_H
#define _LINUX_H

#include <unistd.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/net_switch.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>


#include "switch.h"
#include "if_generic.h"
#include "swsock.h"
#include "util_linux.h"
#include "sw_api.h"

#define SWLINUX_CTX(sw_ops) ((struct linux_context *)(sw_ops))

#define VLAN_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (__ctx->vlan_sfd != -1) {\
		__sock_fd = __ctx->vlan_sfd;\
		break;\
	}\
	__sock_fd = socket(AF_INET, SOCK_STREAM, 0); \
	if (__sock_fd == -1) \
		return -1; \
} while(0)

#define VLAN_SOCK_CLOSE(__ctx, __sock_fd) do {\
	int tmp_errno = errno;		\
	if (__sock_fd != __ctx->vlan_sfd)\
		close(__sock_fd);	\
	errno = tmp_errno;		\
} while (0)

#define BRIDGE_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (__ctx->bridge_sfd != -1) {\
		__sock_fd = __ctx->bridge_sfd;\
		break;\
	}\
	__sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0); \
	if (__sock_fd == -1) \
		return -1; \
} while(0)

#define BRIDGE_SOCK_CLOSE(__ctx, __sock_fd) do {\
	int tmp_errno = errno;		\
	if (__sock_fd != __ctx->bridge_sfd)\
		close(__sock_fd);	\
	errno = tmp_errno;		\
} while (0)

#define IF_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (__ctx->if_sfd != -1) {\
		__sock_fd = __ctx->if_sfd;\
		break;\
	}\
	__sock_fd = socket(AF_INET, SOCK_DGRAM, 0); \
	if (__sock_fd == -1) \
		return -1; \
} while(0)

#define IF_SOCK_CLOSE(__ctx, __sock_fd) do {\
	int tmp_errno = errno;		\
	if (__sock_fd != __ctx->if_sfd)\
		close(__sock_fd);	\
	errno = tmp_errno;		\
} while (0)

#define LINUX_DEFAULT_BRIDGE		"vlan1"
#define LINUX_DEFAULT_VLAN		1

struct linux_context {
	struct switch_operations sw_ops;

	/* Socked descriptors used for communication with
	 * 8021q and bridge module */
	int vlan_sfd, bridge_sfd, if_sfd;
};

extern struct linux_context lnx_ctx;

#endif

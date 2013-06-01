#ifndef _LINUX_H
#define _LINUX_H

#include <unistd.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <sys/ioctl.h>


#include "switch.h"
#include "if_generic.h"
#include "swsock.h"
#include "sw_api.h"
#include "util.h"

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

#define init_vlan_bitmap(__bitmap) do {	\
	int i = 0;			\
	for (i = 0; i < 512; i++)	\
		__bitmap[i] = 0xFF;	\
} while(0)

#define LINUX_DEFAULT_BRIDGE		"vlan1"
#define LINUX_DEFAULT_VLAN		1
#define VLAN_NAME_LEN			9

struct linux_context {
	struct switch_operations sw_ops;

	/* Socked descriptors used for communication with
	 * 8021q and bridge module */
	int vlan_sfd, bridge_sfd, if_sfd;
};

extern struct linux_context lnx_ctx;


/********* util_linux.c ***********/

/* Check if there is a VLAN in switch */
extern int has_vlan(int vlan);

/* Check if the switch has a VLAN interface */
extern int has_vlan_if(int vlan);

/* Set the interface type to "Routed" */
extern int if_no_switchport(struct linux_context *lnx_ctx, int ifindex, int mode);

/* Set the interface mode to "Access" */
extern int if_mode_access(struct linux_context *lnx_ctx, int ifindex);

/* Set the interface mode to "Trunk" */
extern int if_mode_trunk(struct linux_context *lnx_ctx, int ifindex);



/* Bridge related functions */
/* Add a new bridge for a certain VLAN */
extern int br_add(struct linux_context *lnx_ctx, int vlan_id);

/* Remove a bridge created for a certain VLAN */
extern int br_remove(struct linux_context *lnx_ctx, int vlan_id);

/* Add a virtual interface to a bridge */
extern int br_add_if(struct linux_context *lnx_ctx, int vlan_id, int ifindex);

/* Remove a virtual interface from a bridge */
extern int br_remove_if(struct linux_context *lnx_ctx, int vlan_id, int ifindex);

/* Set the ageing time for a certain bridge */
extern int br_set_age_time(struct linux_context *lnx_ctx, int vlan, int age_time);



/* 8021q related functions */
/* Create a virtual interface for a certain VLAN */
extern int create_vif(struct linux_context *lnx_ctx, char *if_name, int vlan_id);

/* Remove a virtual interface created for a certain VLAN */
extern int remove_vif(struct linux_context *lnx_ctx, char *if_name);

/* Create virtual interfaces for a trunk interface */
int add_vifs_to_trunk(struct linux_context *lnx_ctx, int ifindex);

/* Remove all the virtual interfaces of a trunk interface */
extern int remove_vifs_from_trunk(struct linux_context *lnx_ctx, int ifindex);

#endif

#include "linux.h"

static int linux_init(struct switch_operations *sw_ops)
{
	return 0;
}

static int if_add(struct switch_operations *sw_ops, int ifindex, int mode)
{
	struct ifreq ifr;
	int ret, bridge_sfd, if_sfd;
	char *if_name = NULL;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = { BRCTL_ADD_IF, ifindex, 0, 0 };


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_name = if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);


	/* Add the new interface to the default bridge */
	strncpy(ifr.ifr_name, LINUX_DEFAULT_BRIDGE, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);

	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);

	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

static int if_remove(struct switch_operations *sw_ops, int ifindex)
{
	struct ifreq ifr;
	int ret, bridge_sfd, if_sfd;
	char *if_name = NULL;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = { BRCTL_DEL_IF, ifindex, 0, 0 };


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_name = if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);


	/* Remove the new interface from the default bridge */
	strncpy(ifr.ifr_name, LINUX_DEFAULT_BRIDGE, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);

	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);

	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	int ret, bridge_sfd;
	char name[9];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	sprintf(name, "vlan%d", vlan);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRADDBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

static int vlan_del(struct switch_operations *sw_ops, int vlan)
{
	int ret, bridge_sfd;
	char name[9];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	sprintf(name, "vlan%d", vlan);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRDELBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

static int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex)
{
	struct vlan_ioctl_args vrequest;
	int ret, vlan_sfd;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	VLAN_SOCK_OPEN(lnx_ctx, vlan_sfd);
	ret = 0;
	VLAN_SOCK_CLOSE(lnx_ctx, vlan_sfd);

	return ret;
}

static int vif_del(struct switch_operations *sw_ops, int vlan)
{
	struct vlan_ioctl_args vrequest;
	int ret, vlan_sfd;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	VLAN_SOCK_OPEN(lnx_ctx, vlan_sfd);
	ret = 0;
	VLAN_SOCK_CLOSE(lnx_ctx, vlan_sfd);

	return ret;
}

struct linux_context lnx_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init	= linux_init,

		.if_add		= if_add,
		.if_remove	= if_remove,

		.vlan_add	= vlan_add,
		.vlan_del	= vlan_del,
		.vif_add	= vif_add,
		.vif_del	= vif_del
	},
	.vlan_sfd	= -1,
	.bridge_sfd	= -1,
	.if_sfd		= -1
};

#include "linux.h"

static int linux_init(struct switch_operations *sw_ops)
{
	return 0;
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
		.backend_init = linux_init,

		.vlan_add = vlan_add,
		.vlan_del = vlan_del,
		.vif_add = vif_add,
		.vif_del = vif_del
	},
	.vlan_sfd = -1,
	.bridge_sfd = -1
};

#include <linux/net_switch.h>
#include <errno.h>

#include "shared.h"
#include "lisa.h"

static int if_add(struct switch_operations *sw_ops, int ifindex, int mode)
{
	struct swcfgreq swcfgr;
	int ret, sock_fd, ioctl_errno;
	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	SW_SOCK_OPEN(uc, sock_fd);

	swcfgr.cmd = SWCFG_ADDIF;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.switchport = mode;
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(uc, sock_fd);

	errno = ioctl_errno;
	return ret;
}

static int if_remove(struct switch_operations *sw_ops, int ifindex)
{
	struct swcfgreq swcfgr;
	int ret, sock_fd, ioctl_errno;
	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	SW_SOCK_OPEN(uc, sock_fd);

	swcfgr.cmd = SWCFG_DELIF;
	swcfgr.ifindex = ifindex;
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(uc, sock_fd);

	errno = ioctl_errno;
	return ret;
}

static int get_vlan_desc(struct switch_operations *sw_ops, int vlan, char *desc)
{
	return shared_get_vlan_desc(vlan, desc);
}

static int vlan_rename(struct switch_operations *sw_ops, int vlan, char *desc)
{
	int rc;

	if (sw_invalid_vlan(vlan)) {
		rc = -1;
		errno = EINVAL;
	}
	else if (sw_is_default_vlan(vlan)) {
		rc = -1;
		errno = EPERM;
	}
	else
		rc = shared_set_vlan_desc(vlan, desc);

	return rc;
}

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	struct swcfgreq swcfgr;
	int rc, sock_fd, ioctl_errno;
	char desc[SW_MAX_VLAN_NAME];

	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_ADDVLAN;

	SW_SOCK_OPEN(uc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(uc, sock_fd); /* this can overwrite ioctl errno */

	/* Add default description for newly added vlan. */
	if (rc == 0) {
		__default_vlan_name(desc, vlan);
		rc = shared_set_vlan_desc(vlan, desc);
		if (rc) {
			lisa_ctx.sw_ops.vlan_del(&lisa_ctx.sw_ops, vlan);
			goto exit;
		}
	}

	errno = ioctl_errno;
exit:
	return rc;
}

int vlan_del (struct switch_operations *sw_ops, int vlan)
{
	int rc, sock_fd, ioctl_errno;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	rc = shared_del_vlan(vlan);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_DELVLAN;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno */

	errno = ioctl_errno;

	return rc;
}

/* TODO implement switch API with lisa module */
struct lisa_context lisa_ctx = {
	.sock_fd = -1,
	.sw_ops = (struct switch_operations) {
		.if_add = if_add,
		.if_remove = if_remove,
		.vlan_add = vlan_add,
		.vlan_del = vlan_del,
		.vlan_rename = vlan_rename,
		.get_vlan_desc = get_vlan_desc
	}
};



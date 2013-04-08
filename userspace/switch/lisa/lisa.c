#include <linux/net_switch.h>
#include <errno.h>

#include "shared.h"
#include "lisa.h"

static int if_add(struct switch_operations *sw_ops, int ifindex, int mode)
{
	struct swcfgreq swcfgr;
	int ret, sock_fd;
	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	SW_SOCK_OPEN(uc, sock_fd);

	swcfgr.cmd = SWCFG_ADDIF;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.switchport = mode;
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	sw_ops->sw_errno = errno;

	SW_SOCK_CLOSE(uc, sock_fd);

	return ret;
}

static int if_remove(struct switch_operations *sw_ops, int ifindex)
{
	struct swcfgreq swcfgr;
	int ret, sock_fd;
	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	SW_SOCK_OPEN(uc, sock_fd);

	swcfgr.cmd = SWCFG_DELIF;
	swcfgr.ifindex = ifindex;
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);

	SW_SOCK_CLOSE(uc, sock_fd);

	return ret;
}

static int get_vlan_desc(struct switch_operations *sw_ops, int vlan, char *desc)
{
	int rc;

	rc = shared_get_vlan_desc(vlan, desc);
	sw_ops->sw_errno = rc;

	return rc;
}

/* FIXME Should rename check if vlan is present in vdb or should assume that
 * shared mem is consistent with vdb? */
static int vlan_rename(struct switch_operations *sw_ops, int vlan, char *desc)
{
	int rc;

	if (sw_invalid_vlan(vlan))
		rc = -EINVAL;
	else if (sw_is_default_vlan(vlan))
		rc = -EPERM;
	else
		rc = shared_set_vlan_desc(vlan, desc);
	sw_ops->sw_errno = rc;

	return rc;
}

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	struct swcfgreq swcfgr;
	int rc, sock_fd, ioctl_errno;
	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_ADDVLAN;

	SW_SOCK_OPEN(uc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	ioctl_errno = errno;
	SW_SOCK_CLOSE(uc, sock_fd); /* this can overwrite ioctl errno */

	if (rc == -1 && ioctl_errno == EACCES)
		rc = -EACCES;

	if (ioctl_errno == EEXIST)
		rc = -EEXIST;

	sw_ops->sw_errno = rc;
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

	sw_ops->sw_errno = ioctl_errno;

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



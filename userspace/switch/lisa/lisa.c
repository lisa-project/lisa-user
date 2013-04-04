#include <linux/net_switch.h>
#include <errno.h>

#include "shared.h"
#include "lisa.h"

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

/* TODO implement switch API with lisa module */
struct lisa_context lisa_ctx = {
	.sock_fd = -1,
	.sw_ops = (struct switch_operations) {
		.vlan_add = vlan_add,
		.vlan_rename = vlan_rename,
		.get_vlan_desc = get_vlan_desc
	}
};



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

/* TODO implement switch API with lisa module */
struct lisa_context lisa_ctx = {
	.sw_ops = (struct switch_operations) {
		.vlan_rename = vlan_rename,
		.get_vlan_desc = get_vlan_desc
	}
};



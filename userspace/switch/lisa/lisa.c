#include <linux/net_switch.h>
#include <errno.h>

#include "switch.h"
#include "util_lisa.h"
#include "lisa.h"

static int backend_init(struct switch_operations *sw_ops)
{
	return 0;
}

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
	int rc, sock_fd;
	char desc[SW_MAX_VLAN_NAME];

	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_ADDVLAN;

	SW_SOCK_OPEN(uc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(uc, sock_fd); /* this can overwrite ioctl errno */

	/* Add default description for newly added vlan. */
	if (rc == 0) {
		__default_vlan_name(desc, vlan);
		rc = shared_set_vlan_desc(vlan, desc);
		if (rc) {
			sw_ops->vlan_del(sw_ops, vlan);
			goto exit;
		}
	}

exit:
	return rc;
}

static int vlan_del(struct switch_operations *sw_ops, int vlan)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	rc = shared_del_vlan(vlan);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_DELVLAN;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno */

	return rc;
}

static int if_add_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_ADDTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno*/

	return rc;
}

static int if_set_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_SETTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno*/

	return rc;
}

static int if_set_mode (struct switch_operations *sw_ops, int ifindex,
	int mode, int flag)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	rc = 0;

	swcfgr.cmd = mode;
	swcfgr.ifindex = ifindex;

	switch (mode) {
		case SWCFG_SETTRUNK:
			swcfgr.ext.trunk = flag;
			break;
		case SWCFG_SETACCESS:
			swcfgr.ext.access = flag;
			break;
		default:
			rc = -1;
			goto out;
	}

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno*/

out:
	return rc;
}

static int if_del_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_DELTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno*/

	return rc;
}

static int if_get_type(struct switch_operations *sw_ops, int ifindex, int *type)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETIFTYPE;
	swcfgr.ifindex = ifindex;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd); /* this can overwrite ioctl errno*/

	*type = swcfgr.ext.switchport;

	return rc;
}

static int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_ADDVIF;
	swcfgr.vlan = vlan;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	*ifindex = swcfgr.ifindex;

	return rc;
}

static int vif_del(struct switch_operations *sw_ops, int vlan)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_DELVIF;
	swcfgr.vlan = vlan;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int if_set_port_vlan(struct switch_operations *sw_ops, int ifindex, int vlan)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_SETPORTVLAN;
	swcfgr.ifindex = ifindex;
	swcfgr.vlan = vlan;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int get_vlan_interfaces(struct switch_operations *sw_ops, int vlan,
		int *ifindexes, int *no_ifs)
{
	int sock_fd, vlif_no,i;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETVLANIFS;
	swcfgr.vlan = vlan;

	SW_SOCK_OPEN(lc, sock_fd);
	vlif_no = buf_alloc_swcfgr(&swcfgr, sock_fd);
	SW_SOCK_CLOSE(lc, sock_fd);

	if(vlif_no < 0)
		return -1;

	vlif_no /= sizeof(int);
	*no_ifs = vlif_no;

	for (i = 0; i < vlif_no; ++i) {
		ifindexes[i] = ((int *)swcfgr.buf.addr)[i];
	}

	free(swcfgr.buf.addr);
	return 0;
}

static int get_if_list(struct switch_operations *sw_ops, int type,
	int *ifindexes, int *size)
{
	int rc, sock_fd, i;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);
	struct net_switch_device *devs;

	swcfgr.cmd = SWCFG_GETIFLIST;
	swcfgr.ext.switchport = type;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = buf_alloc_swcfgr(&swcfgr, sock_fd);
	SW_SOCK_CLOSE(lc, sock_fd);

	if (rc < 0)
		return -1;

	/* Get the array of devices from response */
	devs = (struct net_switch_device *)swcfgr.buf.addr;
	*size = rc / sizeof(struct net_switch_device);

	for (i = 0; i < *size; i++)
		ifindexes[i] = devs[i].ifindex;

	free(devs);
	return 0;
}

static int if_set_desc(struct switch_operations *sw_ops, int ifindex, char *desc)
{
	return shared_set_if_desc(ifindex, desc);
}

static int if_get_desc(struct switch_operations *sw_ops, int ifindex, char *desc)
{
	return shared_get_if_desc(ifindex, desc);
}

static int set_age_time(struct switch_operations *sw_ops, int age_time)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_SETAGETIME;
	swcfgr.ext.nsec = age_time;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return ret;
}

static int get_age_time(struct switch_operations *sw_ops, int *age_time)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETAGETIME;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	*age_time = swcfgr.ext.nsec;

	return ret;
}

static int mrouters_get(struct switch_operations *sw_ops, int vlan,
		struct list_head *mrouters)
{
	int sock_fd, vlif_no, i;
	struct net_switch_mrouter_e *entry;
	struct net_switch_mrouter ret_value;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETMROUTERS;
	swcfgr.vlan = vlan;

	SW_SOCK_OPEN(lc, sock_fd);
	vlif_no = buf_alloc_swcfgr(&swcfgr, sock_fd);
	SW_SOCK_CLOSE(lc, sock_fd);

	if(vlif_no < 0)
		return -1;

	vlif_no /= sizeof(struct net_switch_mrouter);

	for (i = 0; i < vlif_no; ++i) {
		entry = malloc(sizeof(struct net_switch_mrouter_e));
		if (!entry)
			return -1;

		ret_value = ((struct net_switch_mrouter *)swcfgr.buf.addr)[i];
		entry->ifindex = ret_value.ifindex;
		entry->vlan = ret_value.vlan;

		list_add_tail(&entry->lh, mrouters);
	}

	return 0;
}


static int mrouter_set(struct switch_operations *sw_ops, int vlan,
			int ifindex, int setting)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_SETMROUTER;
	swcfgr.vlan = vlan;
	swcfgr.ext.mrouter = setting;
	swcfgr.ifindex = ifindex;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return ret;
}

/* TODO implement switch API with lisa module */
struct lisa_context lisa_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init = backend_init,

		.if_add = if_add,
		.if_remove = if_remove,
		.vlan_add = vlan_add,
		.vlan_del = vlan_del,
		.vlan_rename = vlan_rename,
		.get_vlan_desc = get_vlan_desc,
		.get_vlan_interfaces = get_vlan_interfaces,
		.get_if_list = get_if_list,

		.if_add_trunk_vlans = if_add_trunk_vlans,
		.if_set_trunk_vlans = if_set_trunk_vlans,
		.if_del_trunk_vlans = if_del_trunk_vlans,

		.if_set_mode = if_set_mode,
		.if_get_type = if_get_type,
		.if_set_desc = if_set_desc,
		.if_get_desc = if_get_desc,
		.if_set_port_vlan = if_set_port_vlan,

		.get_age_time = get_age_time,
		.set_age_time = set_age_time,

		.vif_add = vif_add,
		.vif_del = vif_del,

		.mrouters_get = mrouters_get,
		.mrouter_set = mrouter_set
	},
	.sock_fd = -1
};

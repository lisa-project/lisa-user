/*
 *    This file is part of LiSA Switch
 *
 *    LiSA Switch is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2 of the
 *    License, or (at your option) any later version.
 *
 *    LiSA Switch is distributed in the hope that it will be
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Switch; if not, write to the Free
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 */

#include "switch.h"
#include "util_lisa.h"
#include "lisa.h"

/* In kernel vlan bitmap is kept as forbidden vlan bitmap, so inverse bitmap */
#define inverse_bitmap(vlans)					\
	do {							\
		if ((vlans)) {					\
			int i;					\
			for (i = 0; i < SW_VLAN_BMP_NO; i++)	\
				vlans[i] = ~vlans[i];		\
		}						\
	}while(0)

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

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	struct swcfgreq swcfgr;
	int rc = 0, sock_fd;

	struct lisa_context *uc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_ADDVLAN;

	SW_SOCK_OPEN(uc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(uc, sock_fd);

	return rc;
}

static int vlan_del(struct switch_operations *sw_ops, int vlan)
{
	int rc = 0, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_DELVLAN;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int vlan_set_mac_static(struct switch_operations *sw_ops, int ifindex,
		int vlan, unsigned char *mac)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_MACSTATIC;
	swcfgr.ifindex = ifindex;

	memcpy(swcfgr.ext.mac.addr, mac, ETH_ALEN);

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int vlan_del_mac_static(struct switch_operations *sw_ops, int ifindex,
		int vlan, unsigned char *mac)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.vlan = vlan;
	swcfgr.cmd = SWCFG_DELMACSTATIC;
	swcfgr.ifindex = ifindex;

	memcpy(swcfgr.ext.mac.addr, mac, ETH_ALEN);

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int vlan_del_mac_dynamic(struct switch_operations *sw_ops, int ifindex,
		int vlan)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr = {
		.vlan = vlan,
		.cmd = SWCFG_DELMACDYN,
		.ifindex = ifindex
	};
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int if_add_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	inverse_bitmap(vlans);

	swcfgr.cmd = SWCFG_ADDTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);


	return rc;
}

static int if_set_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	inverse_bitmap(vlans);

	swcfgr.cmd = SWCFG_SETTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int if_set_mode(struct switch_operations *sw_ops, int ifindex, int mode,
		int flag)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	rc = 0;

	swcfgr.ifindex = ifindex;

	switch (mode) {
		case IF_MODE_TRUNK:
			swcfgr.cmd = SWCFG_SETTRUNK;
			swcfgr.ext.trunk = flag;
			break;
		case IF_MODE_ACCESS:
			swcfgr.cmd = SWCFG_SETACCESS;
			swcfgr.ext.access = flag;
			break;
		default:
			rc = -1;
			errno = EINVAL;
			goto out;
	}

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

out:
	return rc;
}

static int if_del_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	inverse_bitmap(vlans);

	swcfgr.cmd = SWCFG_DELTRUNKVLANS;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.bmp = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int if_get_type(struct switch_operations *sw_ops, int ifindex, int *type,
		int *vlan)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETIFTYPE;
	swcfgr.ifindex = ifindex;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	*type = swcfgr.ext.switchport;
	*vlan = swcfgr.vlan;

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

static int if_set_port_vlan(struct switch_operations *sw_ops, int ifindex,
		int vlan)
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
		int **ifindexes, int *no_ifs)
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

	(*ifindexes) = malloc((*no_ifs) * sizeof(int));

	for (i = 0; i < vlif_no; ++i) {
		(*ifindexes)[i] = ((int *)swcfgr.buf.addr)[i];
	}

	free(swcfgr.buf.addr);
	return 0;
}

static int get_vdb(struct switch_operations *sw_ops, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETVDB;
	swcfgr.vlan = 0;
	swcfgr.buf.size = SW_VLAN_BMP_NO;
	swcfgr.buf.addr = (char *)vlans;


	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return rc;
}

static int if_get_cfg (struct switch_operations *sw_ops, int ifindex,
		int *flags,int *access_vlan, unsigned char *vlans)
{
	int rc, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETIFCFG;
	swcfgr.ifindex = ifindex;
	swcfgr.ext.cfg.forbidden_vlans = vlans;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	if (rc < 0)
		return rc;

	*flags = swcfgr.ext.cfg.flags;
	*access_vlan = swcfgr.ext.cfg.access_vlan;

	//inverse_bitmap(vlans);

	return rc;
}

static int get_if_list(struct switch_operations *sw_ops, int type,
	struct list_head *net_devs)
{
	int rc, sock_fd, i, if_no;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);
	struct net_switch_device *dev;
	struct net_switch_dev *resp;

	swcfgr.cmd = SWCFG_GETIFLIST;
	swcfgr.ext.switchport = type;

	SW_SOCK_OPEN(lc, sock_fd);
	rc = buf_alloc_swcfgr(&swcfgr, sock_fd);
	SW_SOCK_CLOSE(lc, sock_fd);

	if (rc < 0)
		return -1;

	/* Get the array of devices from response */
	resp = (struct net_switch_dev *)swcfgr.buf.addr;
	if_no = rc / sizeof(struct net_switch_dev);
	for (i = 0; i < if_no; i++) {
		dev = malloc(sizeof(struct net_switch_device));
		if (!dev) {
			errno = ENOMEM;
			return -1;
		}
		dev->ifindex = resp[i].ifindex;
		dev->type = resp[i].type;
		dev->vlan = resp[i].vlan;
		strcpy(dev->name, resp[i].name);

		list_add_tail(&dev->lh, net_devs);
	}

	return 0;
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
		if (!entry) {
			errno = ENOMEM;
			return -1;
		}

		ret_value = ((struct net_switch_mrouter *)swcfgr.buf.addr)[i];
		entry->ifindex = ret_value.ifindex;
		entry->vlan = ret_value.vlan;

		list_add_tail(&entry->lh, mrouters);
	}

	return 0;
}

static int get_mac(struct switch_operations *sw_ops, int ifindex, int vlan,
			int mac_type, unsigned char *mac_addr,
			struct list_head *macs)
{
	int sock_fd, vlif_no, i;
	struct net_switch_mac_e *entry;
	struct net_switch_mac ret_value;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETMAC;
	swcfgr.ifindex = ifindex;
	swcfgr.vlan = vlan;
	swcfgr.ext.mac.type = mac_type;

	if (mac_addr)
		memcpy(swcfgr.ext.mac.addr, mac_addr, ETH_ALEN);
	else
		memset(swcfgr.ext.mac.addr, 0x0, ETH_ALEN);

	SW_SOCK_OPEN(lc, sock_fd);
	vlif_no = buf_alloc_swcfgr(&swcfgr, sock_fd);
	SW_SOCK_CLOSE(lc, sock_fd);

	if(vlif_no < 0)
		return -1;

	vlif_no /= sizeof(struct net_switch_mac);

	for (i = 0; i < vlif_no; ++i) {
		entry = malloc(sizeof(struct net_switch_mac_e));
		if (!entry)
			return -1;

		ret_value = ((struct net_switch_mac *)swcfgr.buf.addr)[i];
		entry->ifindex = ret_value.ifindex;
		entry->vlan = ret_value.vlan;
		entry->type = ret_value.type;
		memcpy(entry->addr, ret_value.addr, ETH_ALEN);

		list_add_tail(&entry->lh, macs);
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

static int igmp_set(struct switch_operations *sw_ops, int vlan, int snooping)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_SETIGMPS;
	swcfgr.vlan = vlan;
	swcfgr.ext.snooping = snooping;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return ret;
}

static int igmp_get(struct switch_operations *sw_ops, char *buff, int *snooping)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_GETIGMPS;
	swcfgr.buf.addr = buff;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	*snooping = swcfgr.ext.snooping;
	return ret;
}

static int if_enable(struct switch_operations *sw_ops, int ifindex)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_ENABLE_IF;
	swcfgr.ifindex = ifindex;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return ret;
}

static int if_disable(struct switch_operations *sw_ops, int ifindex)
{
	int ret, sock_fd;
	struct swcfgreq swcfgr;
	struct lisa_context *lc = SWLiSA_CTX(sw_ops);

	swcfgr.cmd = SWCFG_DISABLE_IF;
	swcfgr.ifindex = ifindex;

	SW_SOCK_OPEN(lc, sock_fd);
	ret = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(lc, sock_fd);

	return ret;
}

struct lisa_context lisa_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init = backend_init,

		.if_add = if_add,
		.if_remove = if_remove,
		.if_enable = if_enable,
		.if_disable = if_disable,
		.vlan_add = vlan_add,
		.vlan_del = vlan_del,

		.vlan_set_mac_static = vlan_set_mac_static,
		.vlan_del_mac_static = vlan_del_mac_static,
		.vlan_del_mac_dynamic = vlan_del_mac_dynamic,
		.get_mac = get_mac,

		.get_vlan_interfaces = get_vlan_interfaces,
		.get_if_list = get_if_list,
		.get_vdb = get_vdb,

		.if_add_trunk_vlans = if_add_trunk_vlans,
		.if_set_trunk_vlans = if_set_trunk_vlans,
		.if_del_trunk_vlans = if_del_trunk_vlans,

		.if_set_mode = if_set_mode,
		.if_get_type = if_get_type,
		.if_set_port_vlan = if_set_port_vlan,
		.if_get_cfg = if_get_cfg,

		.get_age_time = get_age_time,
		.set_age_time = set_age_time,

		.vif_add = vif_add,
		.vif_del = vif_del,

		.igmp_set = igmp_set,
		.igmp_get = igmp_get,

		.mrouters_get = mrouters_get,
		.mrouter_set = mrouter_set
	},
	.sock_fd = -1
};

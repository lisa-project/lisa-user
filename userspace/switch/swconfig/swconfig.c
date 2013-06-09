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

#include "swconfig.h"
#include <swlib.h>

static int swconfig_init(struct switch_operations *sw_ops)
{
	/* External swlib.so switch device ptr, defined in swlib.h */
	extern struct switch_dev *dev;
	struct swconfig_context *sw_ctx = SWCONFIG_CTX(sw_ops);
	int ret;

	/* get all registered switches, and 'select' the first one */
	dev = swlib_connect(NULL); 

	if(!dev) {
		swlib_free_all(dev);
		return ENODEV;
	}
	/* first switch_dev is 'switch0' */	
	sw_ctx->switch_id = dev->id;  /* = 0 */
	
	/* FIXME: should name, dev_name be stored in shared mm ? */
	sw_ctx->dev_name = malloc(sizeof(dev->dev_name));
	sw_ctx->name = malloc(sizeof(dev->name));
	if(!sw_ctx->name || !sw_ctx->dev_name) {
		swlib_free_all(dev);
		return ENOMEM;
	}
	strncpy(sw_ctx->dev_name, (const char *)dev->dev_name, sizeof(dev->dev_name));
	strncpy(sw_ctx->name, (const char *)dev->name, sizeof(dev->name));
	sw_ctx->ports = dev->ports;
	sw_ctx->vlans = dev->vlans;
	sw_ctx->cpu_port = dev->cpu_port;

	/* enable switch VLAN mode */
	ret = switch_enable_vlans(sw_ctx);
	if(ret)
		return ret;
		
	/* clear SWLIB allocated switch data */	
	swlib_free_all(dev);
	return 0;
}


static int if_add(struct switch_operations *sw_ops, int ifindex, int mode) {
 /* ifindex : switch port number, 0+ */
	int ret = 0;	
	struct swconfig_context *sw_ctx = SWCONFIG_CTX(sw_ops);
	
	if(! (ifindex < sw_ctx->vlans))
		return EINVAL;

	switch(mode) {
		case IF_MODE_ACCESS:
			ret = port_set_untag(SWLIB_UNTAGGED, ifindex, sw_ctx);
			break;
		case IF_MODE_TRUNK:
			ret = port_set_untag(SWLIB_TAGGED, ifindex, sw_ctx);
			break;
		default:
			return EINVAL;
	}

	return 0;
}

static int if_remove(struct switch_operations *sw_ops, int ifindex) {
 /* ifindex : switch port number, 0+ */
	int ret = 0;	
	struct swconfig_context *sw_ctx = SWCONFIG_CTX(sw_ops);
	
	if(! (ifindex < sw_ctx->vlans))
		return EINVAL;

	/* disable port before  */
	if((ret = port_disable(ifindex, sw_ctx)))
		return ret;

	return 0;
}

static int if_enable(struct switch_operations *sw_ops, int ifindex) {
 /* ifindex : switch port number, 0+ */
	int ret = 0;	
	struct swconfig_context *sw_ctx = SWCONFIG_CTX(sw_ops);
	
	if(! (ifindex < sw_ctx->vlans))
		return EINVAL;
	
	if((ret = port_enable(ifindex, sw_ctx)))
		return ret;

	return 0;
}

static int if_disable(struct switch_operations *sw_ops, int ifindex) {
 /* ifindex : switch port number, 0+ */
	int ret = 0;	
	struct swconfig_context *sw_ctx = SWCONFIG_CTX(sw_ops);
	
	if(! (ifindex < sw_ctx->vlans))
		return EINVAL;
	
	if((ret = port_disable(ifindex, sw_ctx)))
		return ret;

	return 0;
}

static int if_set_mode(struct switch_operations *sw_ops, int ifindex, int mode, int flag) {
	return 0;
};
static int if_set_port_vlan(struct switch_operations *sw_ops, int ifindex, int vlan) {
	return 0;
};
static int if_get_cfg(struct switch_operations *sw_ops, int ifindex, int *flags, int *access_vlan, unsigned char *vlans) {
	return 0;
};
static int if_get_type(struct switch_operations *sw_ops, int ifindex, int *type, int *vlan) {
	return 0;
};
static int if_clear_mac(struct switch_operations *sw_ops, int ifindex) {
	return 0;
};

static int if_add_trunk_vlans(struct switch_operations *sw_ops, int ifindex, unsigned char *vlans) {
	return 0;
};
static int if_set_trunk_vlans(struct switch_operations *sw_ops, int ifindex, unsigned char *vlans) {
	return 0;
};
static int if_del_trunk_vlans(struct switch_operations *sw_ops, int ifindex, unsigned char *vlans) {
	return 0;
};

static int get_if_list(struct switch_operations *sw_ops, int type, struct list_head *net_devs) {
	return 0;
};

static int vlan_add(struct switch_operations *sw_ops, int vlan) {
	return 0;
};
static int vlan_del(struct switch_operations *sw_ops, int vlan) {
	return 0;
};

static int vlan_set_mac_static(struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac) {
	return 0;
};
static int vlan_del_mac_static(struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac) {
	return 0;
};
static int vlan_del_mac_dynamic(struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac) {
	return 0;
};

static int get_vlan_interfaces(struct switch_operations *sw_ops, int vlan, int **ifindexes, int *no_ifs) {
	return 0;
};

static int igmp_get(struct switch_operations *sw_ops, char *buff, int *snooping) {
	return 0;
};
static int igmp_set(struct switch_operations *sw_ops, int vlan, int snooping) {
	return 0;
};

static int get_vdb(struct switch_operations *sw_ops, unsigned char *vlans) {
	return 0;
};

static int mrouter_set(struct switch_operations *sw_ops, int vlan, int ifindex, int setting) {
	return 0;
};
static int mrouters_get(struct switch_operations *sw_ops, int vlan, struct list_head *mrouters) {
	return 0;
};

static int get_mac(struct switch_operations *sw_ops, int ifindex, int vlan, int mac_type, unsigned char *optional_mac, struct list_head *macs) {
	return 0;
};

static int get_age_time(struct switch_operations *sw_ops, int *age_time) {
	return 0;
};
static int set_age_time(struct switch_operations *sw_ops, int age_time) {
	return 0;
};

static int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex) {
	return 0;
};
static int vif_del(struct switch_operations *sw_ops, int vlan) {
	return 0;
};

struct swconfig_context sw_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init	= swconfig_init,

		.if_add		= if_add,
		.if_remove	= if_remove,
		.if_enable	= if_enable,
		.if_disable	= if_disable,
		.vlan_add	= vlan_add,
		.vlan_del	= vlan_del,

		.vlan_set_mac_static = vlan_set_mac_static,
		.vlan_del_mac_static = vlan_del_mac_static,
		.vlan_del_mac_dynamic = vlan_del_mac_dynamic,
		.get_mac = get_mac,

		.get_vlan_interfaces = get_vlan_interfaces,
		.get_if_list	= get_if_list,
		.get_vdb	= get_vdb,

		.if_add_trunk_vlans = if_add_trunk_vlans,
		.if_set_trunk_vlans = if_set_trunk_vlans,
		.if_del_trunk_vlans = if_del_trunk_vlans,

		.if_set_mode	= if_set_mode,
		.if_get_type	= if_get_type,
		.if_set_port_vlan = if_set_port_vlan,
		.if_get_cfg	= if_get_cfg,

		.get_age_time	= get_age_time,
		.set_age_time	= set_age_time,

		.vif_add	= vif_add,
		.vif_del	= vif_del,

		.igmp_set	= igmp_set,
		.igmp_get	= igmp_get,

		.mrouters_get	= mrouters_get,
		.mrouter_set	= mrouter_set
	},
	.switch_id	= -1,
	.dev_name = NULL,
	.name = NULL,
	.ports = 0,
	.vlans = 0,
	.cpu_port = -1
};


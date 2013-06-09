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


/* activate changes in HW switch */
int switch_apply(sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "apply");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, NULL);
	return ret;
}

/* reset hw switch */
int switch_reset(sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "reset");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, NULL);
	return ret;
}

/* enable switch VLAN mode */
int switch_enable_vlans(sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "enable_vlan");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, 1);
	return ret;
} 

/* disable switch VLAN mode */
int switch_disable_vlans(sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "enable_vlan");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, 0);
	return ret;

}


/*** VLAN Switch Functions ***/

/* set/get VLAN->Port assignments */

int set_vlan_ports(int vlan, sw_ctx *ctx, int count, struct switch_port **ports){
	return ret;
}

int get_vlan_ports(int vlan, sw_ctx *ctx, int *count, struct switch_port **ports){
	int ret = 0;
	struct switch_attr *a;
	struct sitch_val val;
	if(NULL == sw_ctx->dev) return -1;

	ports = NULL;
	val.port_vlan = vlan;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_VLAN, "ports");
	if(!a)	/* attribute unknown */
		return -1;
	if((ret = swlib_get_attr(sw_ctx->dev, a, &val))<0)
		goto out;
	count = val.len;
	ports = val.value.ports;
out:
	return ret;
}


/*** Switch Port Functions ***/

/* enable/disable switch port */
int port_enable(int id, sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "disable");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, 1);
	return ret;
}

int port_disable(int id, sw_ctx *ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "disable");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, 0);
	return ret;
}

/* set/get switch port untagged */
int port_set_untag(int id, sw_ctx *ctx, int untag){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "untag");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, untag);
	return ret;
}

int port_get_untag(int id, sw_ctx *ctx, int *untag){
	int ret = 0;
	struct switch_attr *a;
	struct sitch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*untag = -1;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "untag");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		untag = val.value.i;
	return ret;
}

/* set/get switch port pvid (native vlan) */
int port_set_pvid(int id, sw_ctx *ctx, int vlan){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "pvid");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, vlan);
	return ret;
}

int port_get_pvid(int id, sw_ctx *ctx, int *vlan){
	int ret = 0;
	struct switch_attr *a;
	struct sitch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*vlan = -1;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "pvid");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		vlan= val.value.i;
	return ret;
}

/* set/get switch port doubletagging */
int port_set_doubletag(int id, sw_ctx *ctx){
	return 0;
}

int port_get_doubletag(int id, sw_ctx *ctx){
	return 0;
}

/* set/get switch port HW group: 1 = lan, 0 = wan */
int port_set_hwgroup(int id, sw_ctx *ctx, int lan){
	return 0;
}

int port_get_hwgroup(int id, sw_ctx *ctx, int lan){
	lan = 1 // lan;
	return 0; 
}

/* get switch port link information */
int port_get_link(int id, char *info, sw_ctx *ctx, char *info){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	info = NULL;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "link");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		info = val.value.s;
	return ret;
}

/* get switch port receive good packet count */
int port_get_recv_good(int id, sw_ctx *ctx, int *count){
	int ret = 0;
	struct switch_attr *a;
	struct sitch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*count = 0;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "recv_good");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		untag = val.value.i;
	return ret;
}

/* get switch port receive bad packet count */
int port_get_recv_bad(int id, sw_ctx *ctx, int *count){
	int ret = 0;
	struct switch_attr *a;
	struct sitch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*count = 0;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "recv_bad");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		untag = val.value.i;
	return ret;
}



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
#include "swlib.h"


/* activate changes in HW switch */
int switch_apply(struct swconfig_context *sw_ctx){
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
int switch_reset(struct swconfig_context *sw_ctx){
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
int switch_enable_vlans(struct swconfig_context *sw_ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "enable_vlan");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, "1");
	return ret;
} 

/* disable switch VLAN mode */
int switch_disable_vlans(struct swconfig_context *sw_ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_GLOBAL, "enable_vlan");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, -1, "0");
	return ret;
}


/*** VLAN Switch Functions ***/

/* set/get VLAN->Port assignments */

int set_vlan_ports(int vlan, struct swconfig_context *sw_ctx, int count, struct switch_port **ports){
	return 0;
}

int get_vlan_ports(int vlan, struct swconfig_context *sw_ctx, int *count, struct switch_port **ports){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	ports = NULL;
	val.port_vlan = vlan;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_VLAN, "ports");
	if(!a)	/* attribute unknown */
		return -1;
	if((ret = swlib_get_attr(sw_ctx->dev, a, &val))<0)
		goto out;
	*count = val.len;
	*ports = val.value.ports;
out:
	return ret;
}


/*** Switch Port Functions ***/

/* enable/disable switch port */
int port_enable(int id, struct swconfig_context *sw_ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "disable");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, "1");
	return ret;
}

int port_disable(int id, struct swconfig_context *sw_ctx){
	int ret = 0;
	struct switch_attr *a;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "disable");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, "0");
	return ret;
}

/* set/get switch port untagged */
int port_set_untag(int id, struct swconfig_context *sw_ctx, int untag){
	int ret = 0;
	struct switch_attr *a;
	char *f;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "untag");
	if(!a)	/* attribute unknown */
		return -1;
	sprintf(f, "%d", untag);
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, f);
	return ret;
}

int port_get_untag(int id, struct swconfig_context *sw_ctx, int *untag){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*untag = -1;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "untag");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		*untag = val.value.i;
	return ret;
}

/* set/get switch port pvid (native vlan) */
int port_set_pvid(int id, struct swconfig_context *sw_ctx, int vlan){
	int ret = 0;
	struct switch_attr *a; 
	char *v;
	if(NULL == sw_ctx->dev) return -1;

	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "pvid");
	if(!a)	/* attribute unknown */
		return -1;
	sprintf(v, "%d", vlan);
	ret = swlib_set_attr_string(sw_ctx->dev, a, id, v);
	return ret;
}

int port_get_pvid(int id, struct swconfig_context *sw_ctx, int *vlan){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*vlan = -1;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "pvid");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		*vlan= val.value.i;
	return ret;
}

/* set/get switch port doubletagging */
int port_set_doubletag(int id, struct swconfig_context *sw_ctx){
	return 0;
}

int port_get_doubletag(int id, struct swconfig_context *sw_ctx){
	return 0;
}

/* set/get switch port HW group: 1 = Lan, 0 = Wan */
int port_set_hwgroup(int id, struct swconfig_context *sw_ctx, int lan){
	return 0;
}

int port_get_hwgroup(int id, struct swconfig_context *sw_ctx, int lan){
	lan = 1;  /* Lan */
	return 0; 
}

/* get switch port state,link,speed,duplex information */
int port_get_link(int id, struct swconfig_context *sw_ctx, char *link){
	int ret = -1;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return ret;

	link = NULL;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "link");
	if(!a)	/* attribute unknown */
		goto out;
	if((ret = swlib_get_attr(sw_ctx->dev, a, &val)))
		goto out;
	if(NULL == (link = malloc(strlen(val.value.s)+1)))
		goto out;
	if(NULL == strcpy(link, val.value.s))
		goto out;
	ret = 0;
out:
	free((void *)val.value.s);
	return ret;
}

/* get switch port receive good packet count */
int port_get_recv_good(int id, struct swconfig_context *sw_ctx, int *count){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*count = 0;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "recv_good");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		*count = val.value.i;
	return ret;
}

/* get switch port receive bad packet count */
int port_get_recv_bad(int id, struct swconfig_context *sw_ctx, int *count){
	int ret = 0;
	struct switch_attr *a;
	struct switch_val val;
	if(NULL == sw_ctx->dev) return -1;

	*count = 0;
	val.port_vlan = id;
	a = swlib_lookup_attr(sw_ctx->dev, SWLIB_ATTR_GROUP_PORT, "recv_bad");
	if(!a)	/* attribute unknown */
		return -1;
	ret = swlib_get_attr(sw_ctx->dev, a, &val);
	if(!ret) 
		*count = val.value.i;
	return ret;
}



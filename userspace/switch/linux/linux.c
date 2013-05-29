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

#include "linux.h"


static int linux_init(struct switch_operations *sw_ops)
{
	return 0;
}

static int if_add(struct switch_operations *sw_ops, int ifindex, int mode)
{
	struct if_data data;
	struct net_switch_device device;
	int ret = 0, if_sfd;
	char if_name[IFNAMSIZE];
	unsigned char vlan_bitmap[512];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);


	/* Add the new interface to the default bridge */
	switch (mode) {
	case IF_MODE_ACCESS:
		ret = br_add_if(lnx_ctx, LINUX_DEFAULT_VLAN, ifindex);
		data.access_vlan = LINUX_DEFAULT_VLAN;
		break;

	case IF_MODE_TRUNK:
		ret = add_vifs_to_trunk(lnx_ctx, ifindex);
		break;

	default:
		return EINVAL;
	}

	/* Create interface specific data */
	strncpy(device.name, if_name, IFNAMSIZ);
	device.ifindex = ifindex;
	device.type = IF_TYPE_SWITCHED;
	data.device = device;

	init_vlan_bitmap(vlan_bitmap);
	data.bitmap = vlan_bitmap;
	data.mode = mode;
	set_if_data(ifindex, data);

	return ret;
}

static int if_remove(struct switch_operations *sw_ops, int ifindex)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;

	/* Get interface data */
	get_if_data(ifindex, &data);


	/* Remove the interface from the default bridge for ACCES mode */
	if (data.device.type == IF_TYPE_ROUTED)
		goto clear_data;

	/* Remove access interface from the default bridge */
	if (IF_MODE_ACCESS == data.mode)
		ret = br_remove_if(lnx_ctx, data.access_vlan, ifindex);

	else
		/* Remove all trunk virtual interfaces */
		ret = remove_vifs_from_trunk(lnx_ctx, ifindex);


	/* Remove the interface specific data */
clear_data:
	del_if_data(ifindex);
	return ret;
}

static int if_enable(struct switch_operations *sw_ops, int ifindex)
{
	int flags, if_sfd, ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Get the current interface flags */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	ret = if_get_flags(ifindex, if_sfd, &flags);
	if(ret)
		return ret;


	/* Set the IFF_UP flag */
	flags |= IFF_UP;
	ret = if_set_flags(ifindex, if_sfd, flags);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	return ret;
}

static int if_disable(struct switch_operations *sw_ops, int ifindex)
{
	int flags, if_sfd, ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Get the current interface flags */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	ret = if_get_flags(ifindex, if_sfd, &flags);
	if(ret)
		return ret;


	/* Reset the IFF_UP flag */
	flags &= ~IFF_UP;
	ret = if_set_flags(ifindex, if_sfd, flags);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	return ret;
}

static int vlan_del(struct switch_operations *sw_ops, int vlan)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct vlan_data *v_data;
	mm_ptr_t ptr, tmp;

	/* Get VLAN data */
	ret = get_vlan_data(vlan, &v_data);
	if (ret)
		return ret;


	/* Remove the virtual interfaces for all the trunk interfaces */
	mm_lock(mm);

	mm_list_for_each_safe(mm, ptr, tmp, mm_ptr(mm, &v_data->vif_list)) {
		struct if_data *vif_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		/* Remove the virtual interface from VLAN's bridge */
		if (br_remove_if(lnx_ctx, vlan, vif_data->device.ifindex))
			continue;

		/* Remove vif */
		ret = remove_vif(lnx_ctx, vif_data->device.name);
		if (ret)
			continue;

		mm_list_del(mm, ptr);
		mm_free(mm, mm_list_entry(ptr, struct if_data, lh));
	}

	mm_unlock(mm);


	/* Delete all access interfaces from this bridge */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		/* Filter only the appropriate access interfaces */
		if (if_data->device.type != IF_TYPE_SWITCHED
				|| if_data->mode != IF_MODE_ACCESS
				|| if_data->access_vlan != vlan)
			continue;

		br_remove_if(lnx_ctx, vlan, if_data->device.ifindex);
	}
	mm_unlock(mm);


	/* Remove the bridge associated with this VLAN if there is no
	 * VLAN interface */
	if (!has_vlan_if(vlan)) {
		ret = br_remove(lnx_ctx, vlan);
		if (ret)
			return ret;
	}

	/* Remove VLAN specific data */
	return del_vlan_data(vlan);
}

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	int ret = 0, if_sfd;
	mm_ptr_t ptr;
	char vif_name[IFNAMSIZE];
	struct net_switch_device vif_device;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Add a bridge for the new VLAN if it hadn't been added before */
	if (!has_vlan_if(vlan)) {
		ret = br_add(lnx_ctx, vlan);
		if (ret)
			return ret;
	}

	/* Create VLAN specific data */
	add_vlan_data(vlan);


	/* Add new virtual interfaces for all the trunk interfaces */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (if_data->device.type != IF_TYPE_SWITCHED)
			continue;

		if (if_data->mode == IF_MODE_TRUNK) {
			/* Create a new virtual interface */
			ret = create_vif(lnx_ctx, if_data->device.name, vlan);
			if (ret)
				continue;

			/* Add virtual interface information to VLAN data */
			sprintf(vif_name, "%s.%d", if_data->device.name, vlan);
			strcpy(vif_device.name, vif_name);
			vif_device.vlan = vlan;

			IF_SOCK_OPEN(lnx_ctx, if_sfd);
			vif_device.ifindex = if_get_index(vif_name, if_sfd);
			IF_SOCK_CLOSE(lnx_ctx, if_sfd);

			add_vif_data(vlan, vif_device);

			/* Add the virtual interface to VLAN's bridge */
			br_add_if(lnx_ctx, vlan, vif_device.ifindex);
			continue;
		}

		/* Add access mode interfaces to the new VLAN */
		if (if_data->mode == IF_MODE_ACCESS &&
				if_data->access_vlan == vlan) {
			br_add_if(lnx_ctx, vlan, if_data->device.ifindex);
		}
	}
	mm_unlock(mm);


	/* Set the ageing time */
	if (SHM->age_time)
		br_set_age_time(lnx_ctx, vlan, SHM->age_time);

	return ret;
}

static int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex)
{
	int ret, if_sfd;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct net_switch_device vif_device;
	struct if_data data;
	char vif_name[IFNAMSIZ];

	/* Check if the VLAN exists */
	if (!has_vlan(vlan)) {
		/* Add a bridge for the new VLAN */
		ret = br_add(lnx_ctx, vlan);
		if (ret)
			return ret;
	}

	/* Create VLAN interface data */
	sprintf(vif_name, "vlan%d", vlan);

	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	vif_device.ifindex = if_get_index(vif_name, if_sfd);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	strncpy(vif_device.name, vif_name, IFNAMSIZ);
	vif_device.type = IF_TYPE_VIF;
	vif_device.vlan = vlan;
	data.device = vif_device;
	set_if_data(vif_device.ifindex, data);

	*ifindex = vif_device.ifindex;

	return ret;
}

static int vif_del(struct switch_operations *sw_ops, int vlan)
{
	int if_sfd, vifindex;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	char vif_name[IFNAMSIZ];


	/* Get VLAN interface index */
	sprintf(vif_name, "vlan%d", vlan);

	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	vifindex = if_get_index(vif_name, if_sfd);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	/* Remove VLAN interface specific data */
	del_if_data(vifindex);


	/* Remove the bridge if the VLAN is not configured */
	if (has_vlan(vlan))
		return 0;
	return br_remove(lnx_ctx, vlan);
}

static int get_vlan_interfaces(struct switch_operations *sw_ops, int vlan, int **ifindexes, int *no_ifs)
{
	int ret = 0, if_sfd, len = 0;
	struct vlan_data *v_data;
	char vif_name[IFNAMSIZE], if_name[IFNAMSIZE];
	mm_ptr_t ptr;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	*no_ifs = 0;

	/* Get VLAN data */
	ret = get_vlan_data(vlan, &v_data);
	if(ret)
		return ret;
	/* Iterate through virtual interfaces and get interface index */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &v_data->vif_list)){
		(*no_ifs)++;

	}

	mm_unlock(mm);

	*ifindexes = malloc((*no_ifs) * sizeof(int));
	*no_ifs = 0;

	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &v_data->vif_list)){
		struct if_data *vif_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		/* Get virtual interface name and extract interface name */
		IF_SOCK_OPEN(lnx_ctx, if_sfd);

		if_get_name(vif_data->device.ifindex, if_sfd, vif_name);

		memset(if_name, 0, IFNAMSIZE);

		len = strchr(vif_name, '.') - vif_name;
		strncpy(if_name, vif_name, len);

		(*ifindexes)[(*no_ifs)++] = if_get_index(if_name, if_sfd);

		IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	}

	mm_unlock(mm);

	return ret;
}

static int set_age_time(struct switch_operations *sw_ops, int age_time)
{
	mm_ptr_t ptr;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Walk through all the VLANs */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		/* Set age time for the VLAN bridge */
		br_set_age_time(lnx_ctx, v_data->vlan_id, age_time);
	}

	mm_unlock(mm);

	/* Save the ageing time inf shared memory */
	SHM->age_time = age_time;

	return 0;
}

static int get_age_time(struct switch_operations *sw_ops, int *age_time)
{
	*age_time = SHM->age_time;
	return 0;
}

static int if_get_type(struct switch_operations *sw_ops, int ifindex,
	int *type, int *vlan)
{
	mm_ptr_t ptr;

	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (if_data->device.ifindex == ifindex) {
			*type = if_data->device.type;
			if (IF_TYPE_VIF == *type)
				*vlan = if_data->device.vlan;
			mm_unlock(mm);

			return 0;
		}
	}

	mm_unlock(mm);

	return ENODEV;
}

static int if_set_mode(struct switch_operations *sw_ops, int ifindex,
	int mode, int flag)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Set interface type to "routed" */
	if (0 == flag)
		return if_no_switchport(lnx_ctx, ifindex, mode);

	switch (mode) {
	case IF_MODE_ACCESS:
		ret = if_mode_access(lnx_ctx, ifindex);
		break;

	case IF_MODE_TRUNK:
		ret = if_mode_trunk(lnx_ctx, ifindex);
		break;

	default:
		return EINVAL;
	}

	return ret;
}

static int if_set_port_vlan(struct switch_operations *sw_ops, int ifindex, int vlan)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;


	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret)
		return ret;

	/* Filter Routed and trunk interfaces */
	if (data.mode != IF_MODE_ACCESS || data.device.type != IF_TYPE_SWITCHED)
		return EINVAL;
	if (data.access_vlan == vlan)
		return 0;


	/* Make sure the interface is not the previous bridge */
	ret = br_remove_if(lnx_ctx, data.access_vlan, ifindex);

	data.access_vlan = vlan;
	del_if_data(ifindex);
	set_if_data(ifindex, data);


	/* Add the interface to the new only if the VLAN exists */
	if (!has_vlan(vlan))
		return 0;
	return br_add_if(lnx_ctx, vlan, ifindex);
}

struct linux_context lnx_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init	= linux_init,

		.if_add		= if_add,
		.if_remove	= if_remove,
		.if_enable	= if_enable,
		.if_disable	= if_disable,

		.if_get_type	= if_get_type,
		.if_set_mode	= if_set_mode,
		.if_set_port_vlan = if_set_port_vlan,

		.vlan_add	= vlan_add,
		.vlan_del	= vlan_del,

		.get_vlan_interfaces = get_vlan_interfaces,

		.vif_add	= vif_add,
		.vif_del	= vif_del,

		.get_age_time = get_age_time,
		.set_age_time = set_age_time
	},
	.vlan_sfd	= -1,
	.bridge_sfd	= -1,
	.if_sfd		= -1
};

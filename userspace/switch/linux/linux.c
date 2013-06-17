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
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	/* Create interface specific data */
	strncpy(device.name, if_name, IFNAMSIZ);
	device.ifindex = ifindex;
	device.type = IF_TYPE_SWITCHED;
	data.device = device;

	data.mode = mode;
	add_if_data(ifindex, data);


	/* Add the new interface to the default bridge */
	switch (mode) {
	case IF_MODE_ACCESS:
		ret = br_add_if(lnx_ctx, LINUX_DEFAULT_VLAN, ifindex);
		data.access_vlan = LINUX_DEFAULT_VLAN;
		break;

	case IF_MODE_TRUNK:
		ret = add_vifs_to_trunk(lnx_ctx, ifindex, NULL);
		break;

	default:
		return EINVAL;
	}

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
		ret = remove_vifs_from_trunk(lnx_ctx, ifindex, NULL);


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
	if (ret)
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
	if (ret)
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
	unsigned char *allowed_vlans;
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

		allowed_vlans = mm_addr(mm, if_data->allowed_vlans);
		if (!sw_bitmap_test(allowed_vlans, vlan))
			continue;

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

static int get_vlan_interfaces(struct switch_operations *sw_ops, int vlan,
		int **ifindexes, int *no_ifs)
{
	int ret = 0, ifs = 0, *ifaces;
	struct vlan_data *v_data;
	mm_ptr_t ptr;
	unsigned char *bitmap;

	/* Get VLAN data */
	ret = get_vlan_data(vlan, &v_data);
	if (ret) {
		*no_ifs = 0;
		return 0;
	}

	/* Iterate through all interfaces and count them */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		/* Count access interfaces */
		if (if_data->mode == IF_MODE_ACCESS || if_data->access_vlan == vlan) {
			ifs++;
			continue;
		}

		/* Count trunk interfaces */
		bitmap = mm_addr(mm, if_data->allowed_vlans);
		if (bitmap != NULL && sw_bitmap_test(bitmap, vlan))
			ifs++;
	}
	mm_unlock(mm);

	ifaces = malloc((ifs) * sizeof(int));
	*no_ifs = ifs;
	ifs = 0;

	mm_lock(mm);
	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (!(if_data->device.type & IF_TYPE_SWITCHED))
			continue;

		/* Count access interfaces */
		if (if_data->mode == IF_MODE_ACCESS || if_data->access_vlan == vlan) {
			ifaces[ifs++] = if_data->device.ifindex;
			continue;
		}

		/* Count trunk interfaces */
		bitmap = mm_addr(mm, if_data->allowed_vlans);
		if (bitmap != NULL && sw_bitmap_test(bitmap, vlan))
			ifaces[ifs++] = if_data->device.ifindex;
	}
	mm_unlock(mm);
	*ifindexes = ifaces;

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
	set_if_data(ifindex, data);


	/* Add the interface to the bridge only if the VLAN exists */
	if (!has_vlan(vlan))
		return 0;
	return br_add_if(lnx_ctx, vlan, ifindex);
}

static int get_vdb(struct switch_operations *sw_ops, unsigned char *vlans)
{
	unsigned char bitmap[SW_VLAN_BMP_NO];
	mm_ptr_t ptr;

	memset(bitmap, 0xFF, SW_VLAN_BMP_NO);


	/* Walk through the vlan_data list */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		sw_bitmap_reset(bitmap, v_data->vlan_id);
	}
	mm_unlock(mm);

	memcpy(vlans, bitmap, SW_VLAN_BMP_NO);

	return 0;
}

static int get_if_list(struct switch_operations *sw_ops, int type,
	struct list_head *net_devs)
{
	struct net_switch_device *dev;
	mm_ptr_t ptr;


	/* Walk throught the list of interfaces */
	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (!(type & if_data->device.type))
			continue;

		/* Copy from shared memory in a new structure */
		dev = malloc(sizeof(struct net_switch_device));
		if (!dev) {
			errno = ENOMEM;
			return -1;
		}
		*dev = if_data->device;
		list_add_tail(&dev->lh, net_devs);
	}
	mm_unlock(mm);

	return 0;
}

static int igmp_set(struct switch_operations *sw_ops, int vlan, int snooping);

static void set_global_igmp(struct switch_operations *sw_ops, int snooping)
{
	int i;
	unsigned char vdb[SW_VLAN_BMP_NO];

	get_vdb(sw_ops, vdb);
	for (i = 1; i < SW_MAX_VLAN; i++) {
		if (!sw_bitmap_test(vdb, i))
			continue;

		/* Set igmp for each VLAN in VDB */
		igmp_set(sw_ops, i, snooping);
	}
}

static int igmp_set(struct switch_operations *sw_ops, int vlan, int snooping)
{
	int ret = 0;
	char file_name[255];
	struct vlan_data *v_data;
	FILE *f = NULL;

	if (vlan == 0) {
		set_global_igmp(sw_ops, snooping);

		/* Change the global flag */
		mm_lock(mm);
		SHM->igmp_snooping = snooping;
		mm_unlock(mm);

		return 0;
	}

	mcast_snoop_file(file_name, vlan);

	/* Open file and write mcast snooping */
	f = fopen(file_name, "r+");
	if (!f)
		return EINVAL;
	fprintf(f, "%d", snooping);


	/* Get VLAN data */
	ret = get_vlan_data(vlan, &v_data);
	if (ret)
		goto out_close;
	v_data->snooping = snooping;
	ret = set_vlan_data(vlan, *v_data);

out_close:
	fclose(f);
	return ret;
}

static int igmp_get(struct switch_operations *sw_ops, char *buff, int *snooping)
{
	unsigned char map[SW_VLAN_BMP_NO];
	mm_ptr_t ptr;

	memset(map, 0, SW_VLAN_BMP_NO);

	mm_lock(mm);
	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		/* set bits for vlans with *disabled* igmp; this way
		 * userspace config builder will know that (a) vlan
		 * exists and (b) vlan has igmp snooping disabled */
		if (!v_data->snooping)
			sw_bitmap_set(map, v_data->vlan_id);
	}

	*snooping = SHM->igmp_snooping;
	mm_unlock(mm);

	memcpy(buff, map, SW_VLAN_BMP_NO);
	return 0;
}

static int mrouter_set(struct switch_operations *sw_ops, int vlan,
		int ifindex, int setting)
{
	int if_sfd, ret = 0;
	FILE *f = NULL;
	char if_name[IFNAMSIZE], file_name[255];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;
	unsigned char *mrouters;

	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);
	sprintf(if_name, "%s.%d", if_name, vlan);

	mcast_router_file(file_name, if_name);


	/* Configure mrouter in sysfs */
	f = fopen(file_name, "r+");
	if (!f)
		return EINVAL;
	ret = fprintf(f, "%d", setting);
	if (ret <= 0) {
		ret = EINVAL;
		goto out_close;
	}


	/* Change the interface's bitmap */
	ret = get_if_data(ifindex, &data);
	if (ret)
		goto out_close;

	mm_lock(mm);
	mrouters = mm_addr(mm, data.mrouters);
	if (setting)
		sw_set_mrouter(mrouters, vlan);
	else
		sw_reset_mrouter(mrouters, vlan);
	mm_unlock(mm);

out_close:
	fclose(f);
	return ret;
}

static int mrouters_get(struct switch_operations *sw_ops, int vlan,
		struct list_head *mrouters)
{
	mm_ptr_t ptr;
	unsigned char *if_mrouters;
	struct net_switch_mrouter_e *entry;

	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *if_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if_mrouters = mm_addr(mm, if_data->mrouters);

		if (!sw_is_mrouter(if_mrouters, vlan))
			continue;

		entry = malloc(sizeof(struct net_switch_mrouter_e));
		if (!entry) {
			errno = ENOMEM;
			mm_unlock(mm);
			return -1;
		}

		entry->ifindex = if_data->device.ifindex;
		entry->vlan = vlan;
		list_add_tail(&entry->lh, mrouters);
	}
	mm_unlock(mm);

	return 0;
}

static int if_add_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;
	unsigned char *bitmap;
	unsigned char aux_bitmap[SW_VLAN_BMP_NO];

	/* Get interface data */
	get_if_data(ifindex, &data);

	mm_lock(mm);

	bitmap = mm_addr(mm, data.allowed_vlans);

	if (data.mode == IF_MODE_ACCESS) {

		sw_bitmap_or(bitmap, vlans, bitmap);
		/* TODO return errno here */

	}
	if (data.mode == IF_MODE_TRUNK) {

		sw_bitmap_xor(bitmap, vlans, aux_bitmap);
		sw_bitmap_and(vlans, aux_bitmap, aux_bitmap);

		/* set new allowed vlans */
		sw_bitmap_or(bitmap, vlans, bitmap);

		mm_unlock(mm);

		add_vifs_to_trunk(lnx_ctx, ifindex, aux_bitmap);

		mm_lock(mm);

	}

	mm_unlock(mm);

	return ret;
}

static int if_set_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;
	unsigned char *bitmap;
	unsigned char aux_bitmap[SW_VLAN_BMP_NO];

	/* Get interface data */
	get_if_data(ifindex, &data);

	mm_lock(mm);

	/*  get allowed vlans bitmap  */
	bitmap = mm_addr(mm, data.allowed_vlans);

	if (data.mode == IF_MODE_ACCESS) {
		memset(bitmap, 0x00, SW_VLAN_BMP_NO);
		sw_bitmap_or(bitmap, vlans, bitmap);
	}
	if (data.mode == IF_MODE_TRUNK) {

		sw_bitmap_xor(bitmap, vlans, aux_bitmap);
		sw_bitmap_and(vlans, aux_bitmap, aux_bitmap);

		mm_unlock(mm);

		add_vifs_to_trunk(lnx_ctx, ifindex, aux_bitmap);

		mm_lock(mm);

		sw_bitmap_xor(bitmap, vlans, aux_bitmap);
		sw_bitmap_and(bitmap,aux_bitmap, aux_bitmap);

		mm_unlock(mm);

		remove_vifs_from_trunk(lnx_ctx, ifindex, aux_bitmap);

		mm_lock(mm);

		memset(bitmap, 0, SW_VLAN_BMP_NO);
		sw_bitmap_or(bitmap, vlans, bitmap);

	}

	mm_unlock(mm);

	return ret;
}

static int if_del_trunk_vlans(struct switch_operations *sw_ops,
	int ifindex, unsigned char *vlans)
{
	int ret = 0;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct if_data data;
	unsigned char *bitmap;
	unsigned char aux_bitmap[SW_VLAN_BMP_NO];

	/* Get interface data */
	get_if_data(ifindex, &data);

	mm_lock(mm);

	bitmap = mm_addr(mm, data.allowed_vlans);

	if (data.mode == IF_MODE_ACCESS) {

		sw_bitmap_xor(bitmap, vlans, aux_bitmap);
		sw_bitmap_and(bitmap, aux_bitmap, bitmap);
		/* TODO return errno here */

	}
	if (data.mode == IF_MODE_TRUNK) {

		sw_bitmap_and(bitmap, vlans, aux_bitmap);

		mm_unlock(mm);

		remove_vifs_from_trunk(lnx_ctx, ifindex, aux_bitmap);

		mm_lock(mm);

		sw_bitmap_xor(bitmap, vlans, aux_bitmap);
		sw_bitmap_and(bitmap, aux_bitmap, bitmap);

	}
	mm_unlock(mm);
	
	return ret;

}

static int if_get_cfg (struct switch_operations *sw_ops, int ifindex,
		int *flags, int *access_vlan, unsigned char *vlans)
{
	int ret = 0;
	struct if_data data;
	unsigned char *allowed_vlans;


	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret)
		return ret;

	*flags = data.mode;
	if (data.mode == IF_MODE_ACCESS)
		*access_vlan = data.access_vlan;
	else {
		mm_lock(mm);

		allowed_vlans = mm_addr(mm, data.allowed_vlans);
		memcpy(vlans, allowed_vlans, SW_VLAN_BMP_NO);

		mm_unlock(mm);
	}

	return ret;
}

static int vlan_set_mac_static(struct switch_operations *sw_ops, int ifindex,
		int vlan, unsigned char *mac)
{
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = {BRCTL_SET_MAC_STATIC, ifindex, VLAN_N_VID,
		(unsigned long) mac};

	return send_mac_cmd(lnx_ctx, ifindex, vlan, args);
}

static int vlan_del_mac_static(struct switch_operations *sw_ops, int ifindex,
		int vlan, unsigned char *mac)
{
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = {BRCTL_DEL_MAC, ifindex, VLAN_N_VID,
		(unsigned long) mac};

	return send_mac_cmd(lnx_ctx, ifindex, vlan, args);
}

static int vlan_del_mac_dynamic(struct switch_operations *sw_ops, int ifindex, int vlan)
{
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = {BRCTL_DEL_MAC, ifindex, VLAN_N_VID, 0};

	return send_mac_cmd(lnx_ctx, ifindex, vlan, args);
}

static int get_mac(struct switch_operations *sw_ops, int ifindex, int vlan,
			int mac_type, unsigned char *mac_addr,
			struct list_head *macs)
{
	int ret = 0, i = 0, mac_no;
	struct net_switch_mac_e *entry;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	struct __fdb_entry** buffer;

	buffer = malloc( sizeof(*buffer) );

	if(!buffer) {
		errno = ENOMEM;
		return -1;
	}

	if(vlan) {
		ret = br_get_all_fdb_entries(lnx_ctx, vlan, (void **)buffer);

		if (ret < 0){
			errno = -ret;
			return -1;
		}

		mac_no = ret;
		for (i = 0; i < mac_no; ++i) {

			entry = malloc(sizeof(struct net_switch_mac_e));
			if (!entry) {
				errno = ENOMEM;
				return -1;
			}

			entry->ifindex = 0;
			entry->vlan = 0;
			entry->type = 0;
			memcpy(entry->addr, (*buffer)[i].mac_addr, ETH_ALEN);

			list_add_tail(&entry->lh, macs);
		}
		free(*buffer);

	}
	return 0;
}

struct linux_context lnx_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init	= linux_init,

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
	.vlan_sfd	= -1,
	.bridge_sfd	= -1,
	.if_sfd		= -1
};

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

/* returns index from vif index */
int if_get_index_from_vif(struct linux_context *lnx_ctx, int vifindex)
{
	int ret = 0, if_sfd, len = 0;
	char vif_name[IFNAMSIZE], if_name[IFNAMSIZE];
	char *poz;

	IF_SOCK_OPEN(lnx_ctx, if_sfd);

	if_get_name(vifindex, if_sfd, vif_name);
	memset(if_name, 0, IFNAMSIZE);
	poz = strchr(vif_name, '.');
	
	if (!poz) {
		IF_SOCK_CLOSE(lnx_ctx, if_sfd);
		return vifindex;
	}
	else {
		len = poz - vif_name;
		strncpy(if_name, vif_name, len);
		ret = if_get_index(if_name, if_sfd);
	}

	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	return ret;
}
/* Returns 1 if the switch has this VLAN, 0 otherwise */
int has_vlan(int vlan)
{
	mm_ptr_t ptr;

	mm_lock(mm);
	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		if (v_data->vlan_id == vlan) {
			mm_unlock(mm);
			return 1;
		}
	}
	mm_unlock(mm);

	return 0;
}

/* Returns 1 if the switch has this VLAN interface, 0 otherwise */
int has_vlan_if(int vlan)
{
	mm_ptr_t ptr;

	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *vif_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (vif_data->device.type == IF_TYPE_VIF &&
			vif_data->device.vlan == vlan) {
			mm_unlock(mm);
			return 1;
		}
	}

	mm_unlock(mm);

	return 0;
}

int br_set_age_time(struct linux_context *lnx_ctx, int vlan, int age_time)
{
	int ret = 0, bridge_sfd;
	struct ifreq ifr;
	unsigned long args[4] = {BRCTL_SET_AGEING_TIME,
		sec_to_jiffies(age_time), 0, 0};
	char br_name[IFNAMSIZ];

	sprintf(br_name, "vlan%d", vlan);
	strcpy(ifr.ifr_name, br_name);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

int br_add(struct linux_context *lnx_ctx, int vlan_id)
{
	int ret = 0, bridge_sfd;
	char name[VLAN_NAME_LEN];

	sprintf(name, "vlan%d", vlan_id);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRADDBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

int br_remove(struct linux_context *lnx_ctx, int vlan_id)
{
	int ret = 0, bridge_sfd;
	char name[9];

	sprintf(name, "vlan%d", vlan_id);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRDELBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

int br_add_if(struct linux_context *lnx_ctx, int vlan_id, int ifindex)
{
	int ret = 0, bridge_sfd;
	struct ifreq ifr;
	char br_name[VLAN_NAME_LEN];
	unsigned long args[4] = { BRCTL_ADD_IF, ifindex, 0, 0 };

	/* Build the name of the bridge */
	sprintf(br_name, "vlan%d", vlan_id);

	/* Add the interface to the bridge */
	strncpy(ifr.ifr_name, br_name, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

int br_remove_if(struct linux_context *lnx_ctx, int vlan_id, int ifindex)
{
	int ret = 0, bridge_sfd;
	struct ifreq ifr;
	unsigned long args[4] = { BRCTL_DEL_IF, ifindex, 0, 0 };
	char br_name[VLAN_NAME_LEN];

	/* Build the name of the bridge */
	sprintf(br_name, "vlan%d", vlan_id);

	/* Remove the interface from the bridge */
	strncpy(ifr.ifr_name, br_name, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

static int manage_vif(struct linux_context *lnx_ctx, char *if_name,
	int vlan_id, int cmd)
{
	int ret = 0, vlan_sfd;
	struct vlan_ioctl_args if_request;

	/* Create a new interface in a given VLAN */
	strcpy(if_request.device1, if_name);
	if (ADD_VLAN_CMD == cmd)
		if_request.u.VID = vlan_id;
	if_request.cmd = cmd;

	VLAN_SOCK_OPEN(lnx_ctx, vlan_sfd);
	ret = ioctl(vlan_sfd, SIOCSIFVLAN, &if_request);
	VLAN_SOCK_CLOSE(lnx_ctx, vlan_sfd);

	return ret;
}

int create_vif(struct linux_context *lnx_ctx, char *if_name, int vlan_id)
{
	return manage_vif(lnx_ctx, if_name, vlan_id, ADD_VLAN_CMD);
}

int remove_vif(struct linux_context *lnx_ctx, char *if_name)
{
	return manage_vif(lnx_ctx, if_name, 0, DEL_VLAN_CMD);
}

int add_vifs_to_trunk(struct linux_context *lnx_ctx, int ifindex,
		unsigned char *bitmap)
{
	int ret = 0, if_sfd;
	char if_name[IFNAMSIZE], vif_name[IFNAMSIZE];
	struct net_switch_device vif_device;
	struct if_data data;
	unsigned char *allowed_vlans;
	mm_ptr_t ptr;


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	get_if_data(ifindex, &data);

	/* Create interfaces for each VLAN in the switch */
	mm_lock(mm);

	allowed_vlans = mm_addr(mm, data.allowed_vlans);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		if (bitmap != NULL && !sw_bitmap_test(bitmap, v_data->vlan_id))
			continue;

		else if (bitmap == NULL && !sw_bitmap_test(allowed_vlans,
					v_data->vlan_id))
			continue;

		/* Use 8021q to add a new interface */
		ret = create_vif(lnx_ctx, if_name, v_data->vlan_id);
		if (ret)
			continue;

		/* Add virtual interface information to VLAN data */
		sprintf(vif_name, "%s.%d", if_name, v_data->vlan_id);
		strcpy(vif_device.name, vif_name);
		vif_device.vlan = v_data->vlan_id;

		IF_SOCK_OPEN(lnx_ctx, if_sfd);
		vif_device.ifindex = if_get_index(vif_name, if_sfd);
		IF_SOCK_CLOSE(lnx_ctx, if_sfd);

		add_vif_data(v_data->vlan_id, vif_device);

		/* Add the new interface to VLAN's bridge */
		br_add_if(lnx_ctx, v_data->vlan_id, vif_device.ifindex);
	}

	mm_unlock(mm);

	return ret;
}

int remove_vifs_from_trunk(struct linux_context *lnx_ctx, int ifindex,
		unsigned char *bitmap)
{
	int ret = 0, if_sfd, vifindex;
	char if_name[IFNAMSIZE], vif_name[IFNAMSIZE];
	unsigned char *allowed_vlans;
	struct if_data data;
	mm_ptr_t ptr;

	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);

	get_if_data(ifindex, &data);

	/* Remove all the virtual interfaces for TRUNK mode */
	mm_lock(mm);

	allowed_vlans = mm_addr(mm, data.allowed_vlans);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		if (bitmap != NULL && !sw_bitmap_test(bitmap, v_data->vlan_id))
			continue;

		else if (bitmap == NULL && !sw_bitmap_test(allowed_vlans,
					v_data->vlan_id))
			continue;


		sprintf(vif_name, "%s.%d", if_name, v_data->vlan_id);
		IF_SOCK_OPEN(lnx_ctx, if_sfd);
		vifindex = if_get_index(vif_name, if_sfd);
		IF_SOCK_CLOSE(lnx_ctx, if_sfd);

		/* Remove vif from VLAN's bridge */
		if (br_remove_if(lnx_ctx, v_data->vlan_id, vifindex))
			continue;

		/* Use 8021q to remove the virtual interface */
		ret = remove_vif(lnx_ctx, vif_name);
		if (ret)
			continue;

		/* Remove virtual interface information from VLAN data */
		del_vif_data(v_data->vlan_id, if_name);
	}

	mm_unlock(mm);

	return ret;
}

int if_no_switchport(struct linux_context *lnx_ctx, int ifindex, int mode)
{
	int ret = 0;
	struct if_data data;


	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret || data.device.type == IF_TYPE_VIF)
		return ret;


	/* Return if the interface is already "routed" */
	if (data.device.type == IF_TYPE_ROUTED)
		return ret;


	/* Remove the interface from the default bridge for ACCES mode */
	if (IF_MODE_ACCESS == data.mode)
		ret = br_remove_if(lnx_ctx, data.access_vlan, ifindex);

	else
		/* Remove all trunk virtual interfaces */
		ret = remove_vifs_from_trunk(lnx_ctx, ifindex, NULL);
	if (ret)
		return ret;


	/* Set the interface type to "Routed" */
	data.device.type = IF_TYPE_ROUTED;
	return set_if_data(ifindex, data);
}

int if_mode_access(struct linux_context *lnx_ctx, int ifindex)
{
	int ret = 0;
	struct if_data data;


	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret || data.device.type == IF_TYPE_VIF)
		return ret;

	/* Return if interface is already access */
	if (data.mode == IF_MODE_ACCESS &&
			data.device.type == IF_TYPE_SWITCHED)
		return 0;


	/* Make sure all trunk virtual interfaces are removed */
	remove_vifs_from_trunk(lnx_ctx, ifindex, NULL);


	/* Add the interface to the default bridge */
	ret = br_add_if(lnx_ctx, LINUX_DEFAULT_VLAN, ifindex);
	if (ret)
		return ret;


	/* Set mode in interface private data */
	data.device.type = IF_TYPE_SWITCHED;
	data.mode = IF_MODE_ACCESS;
	data.access_vlan = LINUX_DEFAULT_VLAN;

	return set_if_data(ifindex, data);
}

int if_mode_trunk(struct linux_context *lnx_ctx, int ifindex)
{
	int ret = 0;
	struct if_data data;


	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret || data.device.type == IF_TYPE_VIF)
		return ret;

	/* Return if interface is already trunk */
	if (data.mode == IF_MODE_TRUNK &&
			data.device.type == IF_TYPE_SWITCHED)
		return 0;

	/* Make sure the interface is not in the default VLAN */
	br_remove_if(lnx_ctx, data.access_vlan, ifindex);

	/* Create virtual interfaces for each VLAN */
	ret = add_vifs_to_trunk(lnx_ctx, ifindex, NULL);
	if (ret)
		return ret;

	/* Set mode in interface private data */
	data.device.type = IF_TYPE_SWITCHED;
	data.mode = IF_MODE_TRUNK;

	return set_if_data(ifindex, data);
}

static int br_get_fdb_entries(struct linux_context *lnx_ctx, int vlan_id,
		void *buff,int maxnum, int offset)
{
	int ret = 0, bridge_sfd;
	struct ifreq ifr;
	char br_name[VLAN_NAME_LEN];
	unsigned long args[4] = { BRCTL_GET_FDB_ENTRIES,
				(unsigned long)(buff+offset), maxnum, offset };

	/* Build the name of the bridge */
	sprintf(br_name, "vlan%d", vlan_id);

	/* Add the interface to the bridge */
	strncpy(ifr.ifr_name, br_name, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;

}

int br_get_all_fdb_entries(struct linux_context *lnx_ctx, int vlan_id, void** buffer )
{
	int ret = 0, offset = 0;
	int maxnum = sysconf(_SC_PAGESIZE)/sizeof(struct __fdb_entry);
	int num_bytes = maxnum * sizeof(struct __fdb_entry);
	int num_entries = 0;

	*buffer = malloc(num_bytes);

	if (*buffer == NULL)
		return -ENOMEM;
	while(1)
	{
		ret = br_get_fdb_entries(lnx_ctx, vlan_id, *buffer, maxnum, offset);
		if (ret < 0)
			return ret;
		num_entries += ret;

		if (ret < maxnum)
			break;
		if (ret == maxnum)
		{
			num_bytes += maxnum * sizeof(struct __fdb_entry);
			*buffer = realloc(*buffer,num_bytes);
			offset += maxnum;

			if (*buffer == NULL)
				return -ENOMEM;
		}

	}

	return num_entries;
}

int get_bridge_port(struct linux_context *lnx_ctx, int *p_index, int ifindex, int vlan)
{
	int ret = 0, if_sfd;
	char if_name[IFNAMSIZE], vif_name[IFNAMSIZE];
	struct if_data data;

	/* Get interface data */
	ret = get_if_data(ifindex, &data);
	if (ret)
		return ret;

	/* Consider the vif ifindex for trunk interfaces */
	if (data.mode == IF_MODE_TRUNK) {
		/* Get the name of the interface */
		IF_SOCK_OPEN(lnx_ctx, if_sfd);
		if_get_name(ifindex, if_sfd, if_name);
		sprintf(vif_name, "%s.%d", if_name, vlan);

		*p_index = if_get_index(vif_name, if_sfd);
		IF_SOCK_CLOSE(lnx_ctx, if_sfd);
	} else
		*p_index = ifindex;

	return 0;
}

int br_get_port_list(struct linux_context *lnx_ctx, int vlan_id,
		int num_ports, int* buffer)
{
	int ret = 0, bridge_sfd;
	struct ifreq ifr;
	char br_name[VLAN_NAME_LEN];
	unsigned long args[4] = { BRCTL_GET_PORT_LIST,
				(unsigned long)buffer, num_ports, 0 };

	/* Build the name of the bridge */
	sprintf(br_name, "vlan%d", vlan_id);

	strncpy(ifr.ifr_name, br_name, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
}

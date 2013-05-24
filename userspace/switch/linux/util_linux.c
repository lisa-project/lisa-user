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

int if_no_switchport(struct linux_context *lnx_ctx, int ifindex, int mode)
{
	int ret = 0;

	return ret;
}

int if_mode_access(struct linux_context *lnx_ctx, int ifindex)
{
	int ret = 0;

	return ret;
}

int if_mode_trunk(struct linux_context *lnx_ctx, int ifindex)
{
	int ret = 0;

	return ret;
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


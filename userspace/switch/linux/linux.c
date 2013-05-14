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

static int add_bridge(struct linux_context *lnx_ctx, int vlan_id)
{
	int ret, bridge_sfd;
	char name[9];

	sprintf(name, "vlan%d", vlan_id);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRADDBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return 0;
}

static int remove_bridge(struct linux_context *lnx_ctx, int vlan_id)
{
	int ret, bridge_sfd;
	char name[9];

	sprintf(name, "vlan%d", vlan_id);

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);
	ret = ioctl(bridge_sfd, SIOCBRDELBR, name);
	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	return ret;
	return 0;
}

static int linux_init(struct switch_operations *sw_ops)
{
	return 0;
}

static int if_add(struct switch_operations *sw_ops, int ifindex, int mode)
{
	struct ifreq ifr;
	struct if_data data;
	struct net_switch_device device;
	int ret, bridge_sfd, if_sfd;
	char if_name[IFNAMSIZE];
	unsigned char vlan_bitmap[512];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = { BRCTL_ADD_IF, ifindex, 0, 0 };


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);


	/* Add the new interface to the default bridge */
	strncpy(ifr.ifr_name, LINUX_DEFAULT_BRIDGE, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);

	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);

	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);


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
	struct ifreq ifr;
	int ret, bridge_sfd, if_sfd;
	char if_name[IFNAMSIZE];
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);
	unsigned long args[4] = { BRCTL_DEL_IF, ifindex, 0, 0 };


	/* Get the name of the interface */
	IF_SOCK_OPEN(lnx_ctx, if_sfd);
	if_get_name(ifindex, if_sfd, if_name);
	IF_SOCK_CLOSE(lnx_ctx, if_sfd);


	/* Remove the new interface from the default bridge */
	strncpy(ifr.ifr_name, LINUX_DEFAULT_BRIDGE, IFNAMSIZ);
	ifr.ifr_data = (char *) args;

	BRIDGE_SOCK_OPEN(lnx_ctx, bridge_sfd);

	ret = ioctl(bridge_sfd, SIOCDEVPRIVATE, &ifr);

	BRIDGE_SOCK_CLOSE(lnx_ctx, bridge_sfd);

	/* Remove the interface specific data */
	del_if_data(ifindex);

	return ret;
}

static int vlan_add(struct switch_operations *sw_ops, int vlan)
{
	int ret;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	/* Add a bridge for the new VLAN */
	ret = add_bridge(lnx_ctx, vlan);
	if (ret)
		return ret;

	/* TODO Create VLAN specific data */

	return ret;
}

static int vlan_del(struct switch_operations *sw_ops, int vlan)
{
	int ret;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	ret = remove_bridge(lnx_ctx, vlan);

	/* TODO Remove VLAN specific data */
	return ret;
}

static int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex)
{
	int ret, vlan_sfd;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	VLAN_SOCK_OPEN(lnx_ctx, vlan_sfd);
	ret = 0;
	VLAN_SOCK_CLOSE(lnx_ctx, vlan_sfd);

	return ret;
}

static int vif_del(struct switch_operations *sw_ops, int vlan)
{
	int ret, vlan_sfd;
	struct linux_context *lnx_ctx = SWLINUX_CTX(sw_ops);

	VLAN_SOCK_OPEN(lnx_ctx, vlan_sfd);
	ret = 0;
	VLAN_SOCK_CLOSE(lnx_ctx, vlan_sfd);

	return ret;
}

struct linux_context lnx_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init	= linux_init,

		.if_add		= if_add,
		.if_remove	= if_remove,

		.vlan_add	= vlan_add,
		.vlan_del	= vlan_del,
		.vif_add	= vif_add,
		.vif_del	= vif_del
	},
	.vlan_sfd	= -1,
	.bridge_sfd	= -1,
	.if_sfd		= -1
};

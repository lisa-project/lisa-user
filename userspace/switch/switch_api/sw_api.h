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
#ifndef _SW_API_H
#define _SW_API_H

#include "list.h"
#include "multiengine.h"

/* If the default interface is given as parameter to a function, the
 * operation will be applied to the first linux implementation.
 * */
#define DEFAULT_SW	-1
#define LINUX_BACKEND	"linux"
#define LiSA_BACKEND	"lisa"

int if_add(int sw_index, char *if_name, int mode);
int if_remove(int sw_index, char *if_name);
int if_set_mode(int sw_index, int if_index, int mode, int flag);
int if_set_port_vlan(int sw_index, int if_index, int vlan);
int if_get_cfg(int sw_index, char *if_name, int *flags, int *access_vlan, unsigned char *vlans);//TODO
int if_get_type_api(int sw_index, char *if_name, int *type, int *vlan);
int if_enable(int sw_index, char *if_name);
int if_disable(int sw_index, char *if_name);
int if_clear_mac(int sw_index, char *if_name);
int if_add_trunk_vlans(int sw_index, char *if_name, char *vlans);
int if_set_trunk_vlans(int sw_index, char *if_name, char *vlans);
int if_del_trunk_vlans(int sw_index, char *if_name, char *vlans);
int get_if_list(int sw_index, int type, struct list_head *net_devs);
int vlan_add(int vlan);
int vlan_del(int vlan);
int vlan_set_mac_static(int sw_index, char *if_name, int vlan, unsigned char *mac);
int vlan_del_mac_static(int sw_index, char *if_name, int vlan, unsigned char *mac);
int vlan_del_mac_dynamic(int sw_index, char *if_name, int vlan, unsigned char *mac);
int get_vlan_interfaces(int vlan, int **ifindexes, int *no_ifs);
int igmp_set(int vlan, int snooping);
int igmp_get(char *buff, int *snooping);
int get_vdb(unsigned char *vlans);
int mrouter_set(int sw_index, int vlan, char *if_name, int setting);
int mrouters_get(int vlan, struct list_head *mrouters);
int get_mac(int sw_index, int vlan, int mac_type, unsigned char *optional_mac, struct list_head *macs);
int get_age_time(int *age_time);
int set_age_time(int *age_time);
int vif_add(int vlan, int *ifindex);
int vif_del(int vlan);


#endif

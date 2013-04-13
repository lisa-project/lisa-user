#ifndef _SW_API_H
#define _SW_API_H

#include "list.h"

#define ETH_ALEN	6

#ifndef IFNAMSIZE
#define IFNAMSIZE	16
#endif

struct net_switch_mrouter_e {
	int ifindex;
	int vlan;
	struct list_head lh;
};

struct net_switch_mac_e {
	unsigned char addr[ETH_ALEN];
	unsigned char type;
	int vlan;
	int ifindex;
	struct list_head lh;
};

/* Generic net device structure */
struct net_switch_device {
	char name[IFNAMSIZE];
	int ifindex;
	int type;
	int vlan; /* used only for VIFs */
};

struct switch_operations {
	int (*backend_init) (struct switch_operations *sw_ops);

	int (*if_add) (struct switch_operations *sw_ops, int ifindex, int mode);

	int (*if_remove) (struct switch_operations *sw_ops, int ifindex);

	/**
	 * @param mode   Switchport mode: access or trunk.
	 * @param flag Specifies if the mode is set to on(flag = 1) or off (flag = 0)
	 */
	int (*if_set_mode) (struct switch_operations *sw_ops, int ifindex, int mode, int flag);

	int (*if_set_port_vlan) (struct switch_operations *sw_ops, int ifindex, int vlan);

	int (*if_set_desc) (struct switch_operations *sw_ops, int ifindex, char *desc);
	/**
	 * @param desc  Get description from mm.
	 */
	int (*if_get_desc) (struct switch_operations *sw_ops, int ifindex, char *desc);
	/**
	 * @param vlans  Vlans are returned using bitmap positive logic.
	 */
	int (*if_get_vlans) (struct switch_operations *sw_ops, int ifindex, unsigned char *vlans);

	/**
	 * @param mode This parameter will return as side effect the type of the interface:
	 * ethernet or a virtual vlan interface
	 */
	int (*if_get_type) (struct switch_operations *sw_ops, int ifindex, int *type);

	int (*if_enable) (struct switch_operations *sw_ops, int ifindex);
	int (*if_disable) (struct switch_operations *sw_ops, int ifindex);

	int (*if_clear_mac) (struct switch_operations *sw_ops, int ifindex);

	int (*if_add_trunk_vlans) (struct switch_operations *sw_ops, int ifindex, unsigned char *vlans);
	int (*if_set_trunk_vlans) (struct switch_operations *sw_ops, int ifindex, unsigned char *vlans);
	int (*if_del_trunk_vlans) (struct switch_operations *sw_ops, int ifindex, unsigned char *vlans);

	/**
	 * Get indexes of interfaces.
	 *
	 * Description may be obtained using if_get_desc().
	 *
	 * @param ifindexes  Array with interfaces' indexes.
	 */
	int (*get_if_list) (struct switch_operations *sw_ops, int type,
		int *ifindexes, int *size);

	int (*vlan_add) (struct switch_operations *sw_ops, int vlan);
	int (*vlan_del) (struct switch_operations *sw_ops, int vlan);
	int (*vlan_rename) (struct switch_operations *sw_ops, int vlan, char *desc);
	int (*vlan_set_mac_static) (struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac);
	/**
	 * @param mac   MAC in binary format.
	 */
	int (*vlan_del_mac_static) (struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac);
	int (*vlan_del_mac_dynamic) (struct switch_operations *sw_ops, int ifindex, int vlan);

	int (*get_vlan_interfaces) (struct switch_operations *sw_ops, int vlan, int *ifindexes, int *no_ifs);

	int (*igmp_enable) (struct switch_operations *sw_ops, int vlan);
	int (*igmp_disable) (struct switch_operations *sw_ops, int vlan);
	int (*igmps_get) (struct switch_operations *sw_ops, char *buff, int *snooping);

	/**
	 * Return a VLAN bitmap.
	 *
	 * VLAN descriptions can be obtained using get_vlan_desc().
	 *
	 * @param vlans  VLAN bitmap.
	 */
	int (*get_vdb) (struct switch_operations *sw_ops, unsigned char *vlans);
	int (*get_vlan_desc) (struct switch_operations *sw_ops, int vlan, char *desc);

	int (*mrouter_set) (struct switch_operations *sw_ops, int vlan, int ifindex, int setting);
	/* Return a list of net_switch_mrouter_e. */
	int (*mrouters_get) (struct switch_operations *sw_ops, int vlan, struct list_head *mrouters);

	/* Return a list of net_switch_mac_e. */
	int (*get_mac) (struct switch_operations *sw_ops, int ifindex, int vlan,
			int mac_type, struct list_head *macs);

	int (*get_age_time) (struct switch_operations *sw_ops, int *age_time);
	int (*set_age_time) (struct switch_operations *sw_ops, int age_time);

	int (*vif_add) (struct switch_operations *sw_ops, int vlan, int *ifindex);
	int (*vif_del) (struct switch_operations *sw_ops, int vlan);

};

#endif

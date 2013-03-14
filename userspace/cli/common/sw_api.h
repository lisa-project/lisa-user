#ifndef _SW_API_H
#define _SW_API_H

struct switch_operations {
	int (*if_add) (struct switch_operations *, int, int);
	int (*if_remove) (struct switch_operations *, int);
	int (*if_set_mode) (struct switch_operations *, int, int);
	int (*if_set_port_vlan) (struct switch_operations *, int, int);
	int (*if_get_config) (struct switch_operations *, int, unsigned char *, char *);
	int (*if_get_type) (struct switch_operations *, int, int *, int *);
	int (*if_enable) (struct switch_operations *, int);
	int (*if_disable) (struct switch_operations *, int);
	int (*if_clear_mac) (struct switch_operations *, int);
	int (*if_add_trunk_vlans) (struct switch_operations *, int, char *);
	int (*if_set_trunk_vlans) (struct switch_operations *, int, char *);
	int (*if_del_trunk_vlans) (struct switch_operations *, int, char *);
	int (*get_if_list) (struct switch_operations *, int, char *);
	int (*set_if_description) (struct switch_operations *, int, char *);

	int (*vlan_add) (struct switch_operations *, int, char *);
	int (*vlan_del) (struct switch_operations *, int);
	int (*vlan_rename) (struct switch_operations *, int, char *);
	int (*vlan_port_add) (struct switch_operations *, int, int);
	int (*vlan_port_del) (struct switch_operations *, int, int);
	int (*vlan_set_mac_static) (struct switch_operations *, int, int, unsigned char *);
	int (*vlan_del_mac_static) (struct switch_operations *, int, int, unsigned char *);
	int (*get_vlan_interfaces) (struct switch_operations *, int, char *);

	int (*igmp_enable) (struct switch_operations *, int);
	int (*igmp_disable) (struct switch_operations *, int);
	int (*igmps_get) (struct switch_operations *, char *, int *);

	int (*get_vdb) (struct switch_operations *, char *, int, char *);
	int (*mrouter_set) (struct switch_operations *, int);
	int (*mrouter_reset) (struct switch_operations *, int);
	int (*mrouters_get) (struct switch_operations *, int, char *);
	int (*get_mac) (struct switch_operations *, int, char *, int, int);
	int (*get_age_time) (struct switch_operations *, int *);
	int (*set_age_time) (struct switch_operations *, int);
	int (*vif_add) (struct switch_operations *, int, int *);
	int (*vif_del) (struct switch_operations *, int);
	int (*del_mac_dynamic) (struct switch_operations *, int, int);

	/* Error code returned by different implementations */
	int errno;
};

/* command: SWCFG_ADDIF */
int if_add(struct switch_operations *sw_ops, int ifindex, int switchport);

/* command: SWCFG_DELIF */
int if_remove(struct switch_operations *sw_ops, int ifindex);

/* command: SWCFG_SETACCESS/SWCFG_SETTRUNK
 * mode represents trunk or acccess */
int if_set_mode(struct switch_operations *sw_ops, int ifindex, int mode);

/* command: SWCFG_SETPORTVLAN */
int if_set_port_vlan(struct switch_operations *sw_ops, int ifindex, int vlan);

/* command: SWCFG_GETIFCFG */
int if_get_config(struct switch_operations *sw_ops, int ifindex, unsigned char *bmp, char *desc);

/* command: SWCFG_GETIFTYPE */
int if_get_type(struct switch_operations *sw_ops, int ifindex, int *vlan, int *ifmode);

/* command: SWCFG_ENABLE_IF */
int if_enable(struct switch_operations *sw_ops, int ifindex);

/* command: SWCFG_DISABLE_IF */
int if_disable(struct switch_operations *sw_ops, int ifindex);

/* command: SWCFG_CLEARMACINT */
int if_clear_mac(struct switch_operations *sw_ops, int ifindex);

/* command: SWCFG_ADDTRUNKVLANS */
int if_add_trunk_vlans(struct switch_operations *sw_ops, int ifindex, char *bmp);

/* command: SWCFG_DELTRUNKVLANS */
int if_del_trunk_vlans(struct switch_operations *sw_ops, int ifindex, char *bmp);

/* command: SWCFG_SETTRUNKVLANS */
int if_set_trunk_vlans(struct switch_operations *sw_ops, int ifindex, char *bmp);

/* command: SWCFG_GETIFLIST */
int get_if_list(struct switch_operations *sw_ops, int switchport, char *buff);

/* command: SWCFG_SETIFDESC */
int set_if_description(struct switch_operations *sw_ops, int ifindex, char *desc);

/* command: SWCFG_ADDVLAN */
int vlan_add(struct switch_operations *sw_ops, int vlan, char *vlan_desc);

/* command: SWCFG_DELVLAN */
int vlan_del(struct switch_operations *sw_ops, int vlan);

/* command: SWCFG_RENAMEVLAN */
int vlan_rename(struct switch_operations *sw_ops, int vlan, char *vlan_desc);

/* command: SWCFG_ADDVLANPORT */
int vlan_port_add(struct switch_operations *sw_ops, int ifindex, int vlan);

/* command: SWCFG_DELVLANPORT */
int vlan_port_del(struct switch_operations *sw_ops, int ifindex, int vlan);

/* command: SWCFG_MACSTATIC */
int vlan_set_mac_static(struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac);

/* command: SWCFG_DELMACSTATIC */
int vlan_del_mac_static(struct switch_operations *sw_ops, int ifindex, int vlan, unsigned char *mac);

/* command: SWCFG_GETVLANIFS */
int get_vlan_interfaces(struct switch_operations *sw_ops, int vlan, char *buff);

/* command: SWCFG_SETIGMPS */
/* a value vlan = 0 enables or disables igmp for all vlans */
int igmp_enable(struct switch_operations *sw_ops, int vlan);
int igmp_disable(struct switch_operations *sw_ops, int vlan);

/* command: SWCFG:GETIGMPS */
int igmps_get(struct switch_operations *sw_ops, char *buff, int *snooping);

/* command: SWCFG_GETVDB */
int get_vdb(struct switch_operations *sw_ops, char *buff, int size, char *vlan_desc);

/* command: SWCFG_SETMROUTER */
int mrouter_set(struct switch_operations *sw_ops, int ifindex);
int mrouter_reset(struct switch_operations *sw_ops, int ifindex);

/* command: SWCFG_GETMROUTERS */
int mrouters_get(struct switch_operations *sw_ops, int vlan, char *buff);

/* command: SWCFG_GETMAC */
int get_mac(struct switch_operations *sw_ops, int ifindex, char *buff, int vlan, int mac_type);

/* command: SWCFG_GETAGETIME */
int get_age_time(struct switch_operations *sw_ops, int *age_time);

/* command: SWCFG_SETAGETIME */
int set_age_time(struct switch_operations *sw_ops, int age_time);

/* command: SWCFG_ADDVIF */
int vif_add(struct switch_operations *sw_ops, int vlan, int *ifindex);

/* command: SWCFG_DELVIF */
int vif_del(struct switch_operations *sw_ops, int vlan);

/* command: SWCFG_DELMACDYN */
int del_mac_dynamic(struct switch_operations *sw_ops, int ifindex, int vlan);

#endif

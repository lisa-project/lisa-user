/*
 * command: SWCFG_ADDIF
 */
int if_add(int ifindex, int switchport);

/* command: SWCFG_DELIF */
int if_remove(int ifindex);

/* command: SWCFG_GETIFLIST */
int get_if_list(int switchport, char *buff);

/* command: SWCFG_SETIFDESC */
int set_if_description(int ifindex, char *desc);

/* command: SWCFG_ADDVLAN */
int vlan_add(int vlan, char *vlan_desc);

/* command: SWCFG_DELVLAN */
int vlan_del(int vlan);

/* command: SWCFG_RENAMEVLAN */
int vlan_rename(int vlan, char *vlan_desc);

/* command: SWCFG_GETVDB */
int get_vdb(char *buff, int size, char *vlan_desc);

/* command: SWCFG_GETVLANIFS */
int get_vlan_interfaces(int vlan, char *buff);

/* command: SWCFG_ADDVLANPORT */
int vlan_port_add(int ifindex, int vlan);

/* command: SWCFG_DELVLANPORT */
int vlan_port_del(int ifindex, int vlan);

/* command: SWCFG_SETACCESS/SWCFG_SETTRUNK
 * mode represents trunk or acccess */
int if_set_mode(int ifindex, int mode);

/* command: SWCFG_SETPORTVLAN */
int if_set_port_vlan(int ifindex, int vlan);

/*
 * command: SWCFG_ADDTRUNKVLANS
 */
int if_add_trunk_vlans(int ifindex, char *bmp);

/*
 * command: SWCFG_DELTRUNKVLANS
 */
int if_set_trunk_vlans(int ifindex, char *bmp);

/*
 * command: SWCFG_SETTRUNKVLANS
 */
int if_set_trunk_vlans(int ifindex, char *bmp);

/* command: SWCFG_SETMROUTER */
int mrouter_set(int ifindex);
int mrouter_reset(int ifindex);

/* command: SWCFG_GETMROUTERS */
int mrouters_get(int vlan, char *buff);

/* command: SWCFG_GETIFTYPE
 * This one is used in "cli/lib/interface.h" in a define
 */
int if_get_type(int ifindex);

/* command: SWCFG_SETIGMPS */
/* a value vlan = 0 enables or disables igmp for all vlans */
int igmp_enable(int vlan);
int igmp_disable(int vlan);

/* command: SWCFG:GETIGMPS */
int igmps_get(char *buff, int *snooping);

/*
 * command: SWCFG_GETMAC
 */
int get_mac(int ifindex, char *buff, int vlan, int mac_type);

/* command: SWCFG_GETAGETIME */
int get_age_time(int *age_time);

/* command: SWCFG_SETAGETIME */
int set_age_time(int age_time);

/* command: SWCFG_CLEARMACINT */
int if_clear_mac(int ifindex);

/* command: SWCFG_MACSTATIC */
int vlan_set_mac_static(int ifindex, int vlan, unsigned char *mac);

/* command: SWCFG_DELMACSTATIC */
int vlan_del_mac_static(int ifindex, int vlan, unsigned char *mac);

/* command: SWCFG_ADDVIF */
int vif_add(int vlan, int *ifindex);

/* command: SWCFG_DELVIF */
int vif_del(int vlan);

/*
 * command: SWCFG_DELMACDYN
 */
int del_mac_dynamic(int ifindex, int vlan);

/*
 * command: SWCFG_GETIFCFG
 */
int if_get_config(int ifindex, unsigned char *bmp, char *desc);


/* command: SWCFG_GETIFTYPE */
int if_get_type(int ifindex, int *vlan, int *ifmode);

/* command: SWCFG_ENABLE_IF */
int if_enable(int ifindex);

/* command: SWCFG_DISABLE_IF */
int if_disable(int ifindex);

#ifndef _NET_SWITCH_H
#define _NET_SWITCH_H

#define SWCFG_ADDIF			0x01	/* add interface to switch */
#define SWCFG_DELIF			0x02	/* remove interface from switch */
#define SWCFG_ADDVLAN		0x03	/* add vlan to vlan database */
#define SWCFG_DELVLAN		0x04	/* delete vlan from vlan database */
#define SWCFG_RENAMEVLAN	0x05	/* rename vlan from vlan database */
#define SWCFG_ADDVLANPORT	0x06	/* add a port to a vlan (trunk mode) */
#define SWCFG_DELVLANPORT 	0x07	/* remove a port from a vlan (trunk mode) */
#define SWCFG_SETTRUNK		0x08	/* put a port in trunk mode */
#define SWCFG_SETPORTVLAN 	0x09	/* add a port in a vlan (non-trunk mode) */

struct net_switch_ioctl_arg {
	unsigned char cmd;
	int vlan;
	char *name;
};

#endif

/*
 *    This file is part of Linux Multilayer Switch.
 *
 *    Linux Multilayer Switch is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Linux Multilayer Switch is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Linux Multilayer Switch; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _NET_SWITCH_H
#define _NET_SWITCH_H

#define SWCFG_ADDIF			0x01	/* add interface to switch */
#define SWCFG_DELIF			0x02	/* remove interface from switch */
#define SWCFG_ADDVLAN		0x03	/* add vlan to vlan database */
#define SWCFG_DELVLAN		0x04	/* delete vlan from vlan database */
#define SWCFG_RENAMEVLAN	0x05	/* rename vlan from vlan database */
#define SWCFG_ADDVLANPORT	0x06	/* add a port to a vlan (trunk mode) */
#define SWCFG_DELVLANPORT 	0x07	/* remove a port from a vlan (trunk mode) */
#define SWCFG_SETACCESS		0x08	/* put a port in access mode */
#define SWCFG_SETTRUNK		0x09	/* put a port in trunk mode */
#define SWCFG_SETPORTVLAN 	0x0a	/* add a port in a vlan (non-trunk mode) */
#define SWCFG_CLEARMACINT	0x0b	/* clear all macs for a given port */
#define SWCFG_SETAGETIME	0x0c	/* set fdb entry aging time interval (in ms) */
#define SWCFG_MACSTATIC		0x0d	/* add static mac */
#define SWCFG_DELMACSTATIC	0x0e	/* delete static mac */
#define SWCFG_GETIFCFG		0x0f	/* get physical port configuration and status */
#define SWCFG_ADDVIF		0x10	/* add virtual interface for vlan */
#define SWCFG_DELVIF		0x11	/* remove virtual interface for vlan */
#define SWCFG_DISABLEPORT	0x12	/* administratively disable port */
#define SWCFG_ENABLEPORT	0x13	/* enable port */
#define SWCFG_SETTRUNKVLANS	0x14	/* set the bitmap of forbidden trunk ports */
#define SWCFG_ADDTRUNKVLANS	0x15	/* add ports to the bitmap of forbidden trunk ports */
#define SWCFG_DELTRUNKVLANS	0x16	/* remove ports from the bitmap of forbidden trunk ports */
#define SWCFG_SETIFDESC		0x17	/* set interface description */
#define SWCFG_SETSPEED		0x18	/* set port speed parameter */
#define SWCFG_SETDUPLEX		0x19	/* set port duplex parameter */
#define SWCFG_GETMAC		0x20	/* fetch mac addresses from the fdb */
#define SWCFG_DELMACDYN		0x21	/* clear dynamic mac addresses from the fdb */
#define SWCFG_ENABLEVIF		0x22	/* administratively enable virtual interface */
#define SWCFG_DISABLEVIF	0x23	/* administratively disable virtual interface */
#define SWCFG_GETAGETIME	0x24	/* get fdb aging time interval */
#define SWCFG_GETVDB		0x30	/* copy the whole vlan database to userspace */
#define SWCFG_STP_PORT_PRIO     0X40    /* Set port priority for stp */
#define SWCFG_STP_PORT_COST     0x41    /* Set port cost for stp */ 
#define SWCFG_STP_SW_PRIO       0x42    /* Set switch priority */
#define SWCFG_STP_HELLO_TIME    0x43    /* Set hello time */
#define SWCFG_STP_FORWARD_DELAY 0x44    /* Set forward delay time */
#define SWCFG_STP_MAX_AGE       0x45    /* Set max age time  */
#define SWCFG_STP_SW_SHOW       0x46    /* Show stp configuration for switch */
#define SWCFG_STP_PORT_SHOW     0x47    /* Show stp configuration for port */
#define SWCFG_STP_ENABLE        0x48    /* Enable stp */
#define SWCFG_STP_DISABLE       0x49    /* Disable stp */
#define SWCFG_STP_SHOW_ST       0x49    /* Show spanning tree. */
#define SWCFG_VTP_SET_DOMAIN	0x50	/* Set VTP administrative domain */
#define SWCFG_VTP_SET_FILE	0x51	/* Configure IFS filesystem file where VTP configuration is stored. */
#define SWCFG_VTP_SET_INTERFACE	0x52	/* Configure interface as the preferred source for the VTP IP updater address */
#define SWCFG_VTP_SET_MODE	0x53	/* Configure VTP device mode */
#define SWCFG_VTP_SET_PASSWORD	0x54	/* Set the password for the VTP administrative domain */
#define SWCFG_VTP_ENABLE_PRUNING 0x55	/* Set the administrative domain to permit pruning */
#define SWCFG_VTP_SET_VERSION	0x56	/* Set the administrative domain to VTP version */

#define VTP_MODE_TRANSPARENT	0X01
#define VTP_MODE_CLIENT		0X02
#define VTP_MODE_SERVER		0X03
#define VTP_TIMESTAMP_SIZE	12

#define SW_PFL_DISABLED     0x01
#define SW_PFL_ACCESS		0x02
#define SW_PFL_TRUNK		0x04
#define SW_PFL_DROPALL		0x08
#define SW_PFL_ADMDISABLED	0x10

#define SW_SPEED_AUTO		0x01
#define SW_SPEED_10		0x02
#define SW_SPEED_100		0x03
#define SW_SPEED_1000		0x04

#define SW_DUPLEX_AUTO		0x01
#define SW_DUPLEX_HALF		0x02
#define SW_DUPLEX_FULL		0x03

#ifdef __KERNEL__
#include <linux/if.h>
#else
#ifndef _LINUX_IF_H
#include <net/if.h>
#include <net/ethernet.h>
#endif
#endif

struct net_switch_ifcfg {
	int flags;
	int access_vlan;
	unsigned char *forbidden_vlans;
	char *description;
	int speed;
	int duplex;
        char prio;
        unsigned int cost;
};

struct net_switch_mac {
	unsigned char addr[ETH_ALEN];
	int addr_type;
	int vlan;
	char port[IFNAMSIZ];
};

struct net_switch_mac_arg {
	unsigned char addr[ETH_ALEN];
	int buf_size;
	int actual_size;
	int addr_type;
	char *buf;
};

#define SW_DEFAULT_AGE_TIME 300

#define SW_MAX_VLAN_NAME	31

struct net_switch_usr_vdb {
	int vlan;
	char name[SW_MAX_VLAN_NAME + 1];
};

struct net_switch_usr_vdb_arg {
	int buf_size;
	int vdb_entries;
	char *buf;
};

struct net_switch_vtp_domain
{
	char length;
	char* domain;
};

struct net_switch_vtp_pass
{
	char* password;
	unsigned char* md5;
};



struct net_switch_ioctl_arg {
	unsigned char cmd;
	char *if_name;
	int vlan;

	char timestamp[VTP_TIMESTAMP_SIZE + 1];

	union {
		int access;
		int trunk;
		int nsec;
		unsigned char *mac;
		unsigned char *bmp;
		char *vlan_desc;
		char *iface_desc;
		int speed;
		int duplex;
		
		/* Stp values. */
	        unsigned  prio;
	        char max_age;
	        char forward_delay;
	        char hello_time;
		
		/* VTP values. */
		char* vtp_domain;
		char vtp_mode;
		struct net_switch_vtp_pass vtp_password;
		char vtp_version;
		
		struct net_switch_ifcfg cfg;
		struct net_switch_mac_arg marg;
		struct net_switch_usr_vdb_arg varg;
	} ext;
};

/* Mac Address types (any, static, dynamic) */
#define SW_FDB_DYN	0
#define SW_FDB_STATIC 1
#define SW_FDB_ANY 2

/* Minimum number a vlan may have */
#define SW_MIN_VLAN 1

/* Maximum number a vlan may have. Note that vlan-related vectors
   must be at least SW_MAX_VLAN + 1 sized, because SW_MAX_VLAN
   should be a valid index.
 */
#define SW_MAX_VLAN 4094

/* Number of octet bitmaps that are necessary to store binary
   information about vlans (i.e. allowed or forbidden on a certain
   port).
 */
#define SW_VLAN_BMP_NO (SW_MAX_VLAN / 8 + 1)

#define sw_valid_vlan(vlan) \
	((vlan) >= SW_MIN_VLAN && (vlan) <= SW_MAX_VLAN)
#define sw_invalid_vlan(vlan) \
	((vlan) < SW_MIN_VLAN || (vlan) > SW_MAX_VLAN)
#define sw_is_default_vlan(vlan) \
	((vlan) == 1 || ((vlan) >= 1002 && (vlan) <= 1005))

#define sw_allow_vlan(bitmap, vlan) ((bitmap)[(vlan) / 8] &= ~(1 << ((vlan) % 8)))
#define sw_forbid_vlan(bitmap, vlan) ((bitmap)[(vlan) / 8] |= (1 << ((vlan) % 8)))
#define sw_forbidden_vlan(bitmap, vlan) ((bitmap)[(vlan) / 8] & (1 << ((vlan) % 8)))
#define sw_allowed_vlan(bitmap, vlan) (!sw_forbidden_vlan(bitmap, vlan))

/* Maximum length of port description */
#define SW_MAX_PORT_DESC	31

#define is_mcast_mac(ptr) \
	((ptr)[0] == 0x01 && (ptr)[1] == 0x00 && (ptr)[2] == 0x5e)
#define is_l2_mac(ptr) \
	((ptr)[0] == 0x01 && (ptr)[1] == 0x80 && (ptr)[2] == 0xc2)
#define is_null_mac(ptr) \
	(((ptr)[0] | (ptr)[1] | (ptr)[2] | (ptr)[3] | (ptr)[4] | (ptr)[5]) == 0)
#define is_bcast_mac(ptr) \
	(((ptr)[0] & (ptr)[1] & (ptr)[2] & (ptr)[3] & (ptr)[4] & (ptr)[5]) == 0xff)

#endif

/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#ifndef _IF_H
#define _IF_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "common.h"
#include "climain.h"
#include "command.h"
#include "list.h"

extern void cmd_sh_int(FILE *, char **);
extern void cmd_int_eth(FILE *, char **);
extern void cmd_int_vlan(FILE *, char **);

/* network interface ioctl's for MII commands */
#ifndef SIOCGMIIPHY
#define SIOCGMIIPHY 0x8947     /* Read from current PHY */
#define SIOCGMIIREG 0x8948     /* Read any PHY register */
#define SIOCSMIIREG 0x8949     /* Write any PHY register */
#endif

/* Basic Mode Control Register */
#define MII_BMCR		0x00
#define  MII_BMCR_RESET		0x8000
#define  MII_BMCR_LOOPBACK	0x4000
#define  MII_BMCR_100MBIT	0x2000
#define  MII_BMCR_AN_ENA	0x1000
#define  MII_BMCR_ISOLATE	0x0400
#define  MII_BMCR_RESTART	0x0200
#define  MII_BMCR_DUPLEX	0x0100
#define  MII_BMCR_COLTEST	0x0080

/* Basic Mode Status Register */
#define MII_BMSR		0x01
#define  MII_BMSR_CAP_MASK	0xf800
#define  MII_BMSR_100BASET4	0x8000
#define  MII_BMSR_100BASETX_FD	0x4000
#define  MII_BMSR_100BASETX_HD	0x2000
#define  MII_BMSR_10BASET_FD	0x1000
#define  MII_BMSR_10BASET_HD	0x0800
#define  MII_BMSR_NO_PREAMBLE	0x0040
#define  MII_BMSR_AN_COMPLETE	0x0020
#define  MII_BMSR_REMOTE_FAULT	0x0010
#define  MII_BMSR_AN_ABLE	0x0008
#define  MII_BMSR_LINK_VALID	0x0004
#define  MII_BMSR_JABBER	0x0002
#define  MII_BMSR_EXT_CAP	0x0001

#define MII_PHY_ID1		0x02
#define MII_PHY_ID2		0x03

/* Auto-Negotiation Advertisement Register */
#define MII_ANAR		0x04
/* Auto-Negotiation Link Partner Ability Register */
#define MII_ANLPAR		0x05
#define  MII_AN_NEXT_PAGE	0x8000
#define  MII_AN_ACK		0x4000
#define  MII_AN_REMOTE_FAULT	0x2000
#define  MII_AN_ABILITY_MASK	0x07e0
#define  MII_AN_FLOW_CONTROL	0x0400
#define  MII_AN_100BASET4	0x0200
#define  MII_AN_100BASETX_FD	0x0100
#define  MII_AN_100BASETX_HD	0x0080
#define  MII_AN_10BASET_FD	0x0040
#define  MII_AN_10BASET_HD	0x0020
#define  MII_AN_PROT_MASK	0x001f
#define  MII_AN_PROT_802_3	0x0001

/* Auto-Negotiation Expansion Register */
#define MII_ANER		0x06
#define  MII_ANER_MULT_FAULT	0x0010
#define  MII_ANER_LP_NP_ABLE	0x0008
#define  MII_ANER_NP_ABLE	0x0004
#define  MII_ANER_PAGE_RX	0x0002
#define  MII_ANER_LP_AN_ABLE	0x0001

#define MII_NO_TRANSCEIVER -1

#define MII_NUM_REGS 32

/* userland net device information */
struct user_net_device {
	char name[IFNAMSIZ];					/* interface name */
	char desc[SW_MAX_PORT_DESC];			/* interface description */
	short type;								/* interface type */
	short flags;							/* interface flags */
	int metric;								/* routing metric */
	int mtu;								/* Maximum transmission unit */
	int tx_queue_len;						/* trasmission queue length */
	struct ifmap map;						/* interface hardware setup details */
	int has_ip;								/* flag (interface has ip address) */
	struct sockaddr addr;					/* IP address */
	struct sockaddr broadaddr;				/* IP broadcast address */
	struct sockaddr netmask;				/* IP network mask */
	struct net_device_stats xstats;			/* interface statistics */
	char mac[ETH_ALEN];						/* MAC address */

	/* MII-specific stuff (registers) */
	int mii_cap;							/* whether interface is mii-capable */
	int phy_id;								/* mii physical id */
	int mii_regs[MII_NUM_REGS];				/* mii registers */
	
	struct list_head lh;					/* next */
};

struct mii_data {
	unsigned short phy_id;
	unsigned short reg_num;
	unsigned short val_in;
	unsigned short val_out;
};


#define print_sockaddr_ip(out, desc, _sockaddr) \
	fprintf(out, "%s: %d.%d.%d.%d  ", \
			desc, \
			_sockaddr.sa_data[2] & 0xFF, \
			_sockaddr.sa_data[3] & 0xFF, \
			_sockaddr.sa_data[4] & 0xFF, \
			_sockaddr.sa_data[5] & 0xFF);
#endif

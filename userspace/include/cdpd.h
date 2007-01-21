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

#ifndef _CDPD_H
#define _CDPD_H

#include <semaphore.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>

#include "common.h"
#include "list.h"

/* integer to alphanumeric mapping */
typedef struct {
	const u_int16_t value;
	const char *description;
} description_table;

/**
 * CDP Frame format:
 *
 * +------------------------+
 * | HEADER					|
 * |------------------------|
 * | version (1 byte)		|
 * | time-to-live (1 byte)	|
 * | checksum (2 bytes)		|
 * +------------------------+
 * | DATA					|
 * |------------------------|
 * | type (2 bytes)			|
 * | length (2 bytes)		|
 * | value (variable)		|
 * +------------------------+
 * | ...					|
 * +------------------------+
 */

#define MAX_CDP_FRAME_SIZE 2048

/* constants for the 'type' field */
#define TYPE_DEVICE_ID			0x0001
#define TYPE_ADDRESS			0x0002
#define TYPE_PORT_ID			0x0003
#define TYPE_CAPABILITIES		0x0004
#define TYPE_IOS_VERSION		0x0005
#define TYPE_PLATFORM			0x0006
#define TYPE_IP_PREFIX			0x0007
#define TYPE_PROTOCOL_HELLO		0x0008
#define TYPE_VTP_MGMT_DOMAIN	0x0009
#define TYPE_NATIVE_VLAN		0x000a
#define TYPE_DUPLEX				0x000b
#define TYPE_VOIP_VLAN_REPLY	0x000e
#define TYPE_VOIP_VLAN_QUERY	0x000f
#define TYPE_MTU				0x0011
#define TYPE_TRUST_BITMAP		0x0012
#define TYPE_UNTRUSTED_COS		0x0013
#define TYPE_SYSTEM_NAME		0x0014
#define TYPE_SYSTEM_OID			0x0015
#define TYPE_MGMT_ADDR			0x0016
#define TYPE_LOCATION			0x0017

/* description table for field types */
static const description_table field_types[] = {
	{ TYPE_DEVICE_ID,		"Device ID" },
	{ TYPE_ADDRESS,			"Addresses" },
	{ TYPE_PORT_ID,			"Port ID" },
	{ TYPE_CAPABILITIES,	"Capabilities" },
	{ TYPE_IOS_VERSION,		"Software version" },
	{ TYPE_PLATFORM,		"Platform" },
	{ TYPE_IP_PREFIX,		"IP Prefix/Gateway" },
	{ TYPE_PROTOCOL_HELLO,	"Protocol Hello" },
	{ TYPE_VTP_MGMT_DOMAIN, "VTP Management Domain" },
	{ TYPE_NATIVE_VLAN,		"Native VLAN" },
	{ TYPE_DUPLEX,			"Duplex" },
	{ TYPE_VOIP_VLAN_REPLY, "VoIP VLAN Reply" },
	{ TYPE_VOIP_VLAN_QUERY,	"VoIP VLAN Query" },
	{ TYPE_MTU,				"MTU" },
	{ TYPE_TRUST_BITMAP,	"Trust Bitmap" },
	{ TYPE_UNTRUSTED_COS,	"Untrusted Port CoS" },
	{ TYPE_SYSTEM_NAME,		"System Name" },
	{ TYPE_SYSTEM_OID,		"System Object ID" },
	{ TYPE_MGMT_ADDR, 		"Management Address" },
	{ TYPE_LOCATION,		"Location" },
	{ 0,					NULL },
};

/* possible values for the protocol type in address fields */
#define PROTO_TYPE_NLPID		0x01
#define PROTO_TYPE_802_2		0x02

/* description table for protocol types */
static const description_table proto_types[] = {
	{ PROTO_TYPE_NLPID,		"NLPID" },
	{ PROTO_TYPE_802_2,		"802.2" },
	{ 0,					NULL },
};

/* possible values for the protocol value in address fields */
#define PROTO_ISO_CLNS 			0x81  /* protocol type 3D 1 */
#define PROTO_IP				0xCC  /* protocol type 3D 1 */
/* for 802.2 the protocol value is 0xAAAA 0300 0000 followed by one of: */
#define PROTO_IPV6				0x0800 /* protocol type 3D 2 */
#define PROTO_DECNET_PHASE_IV 	0x6003 /* protocol type 3D 2 */
#define PROTO_APPLETALK			0x809B /* protocol type 3D 2 */
#define PROTO_NOVELL_IPX 		0x8137 /* protocol type 3D 2 */
#define PROTO_BANYAN_VINES		0x80C4 /* protocol type 3D 2 */
#define PROTO_XNS				0x0600 /* protocol type 3D 2 */
#define PROTO_APOLLO_DOMAIN		0x8019 /* protocol type 3D 2 */

/* description table for protocol values */
static const description_table proto_values[] = {
	{ PROTO_ISO_CLNS,			"ISO CLNS" },
	{ PROTO_IP,					"IPv4" },
	{ PROTO_IPV6,				"IPv6" },
	{ PROTO_DECNET_PHASE_IV,	"DECNET Phase IV" },
	{ PROTO_APPLETALK,			"AppleTalk" },
	{ PROTO_NOVELL_IPX,			"Novell IPX" },
	{ PROTO_BANYAN_VINES,		"Banyan Vines" },
	{ PROTO_XNS,				"XNS" },
	{ PROTO_APOLLO_DOMAIN,		"Apollo Domain" },
	{ 0,						NULL },
};

/* constants for device capabilities (capability masks) */
#define CAP_L3R				0x01	/* device is a layer 3 router */
#define CAP_L2TB			0x02	/* device is a layer 2 transparent bridge */
#define CAP_L2SRB			0x04	/* device is a layer 2 source-route bridge */
#define CAP_L2SW			0x08	/* device is a layer 2 switch (non-spanning tree) */
#define CAP_L3HOST			0x10	/* device is a layer 3 (non routing) host */
#define CAP_IGMP			0x20	/* device is IGMP capable */
#define CAP_L1				0x40	/* device is a layer 1 repeater */

/* description table for device capabilities */
static const description_table device_capabilities_brief[] = {
	{ CAP_L3R,				"R" },
	{ CAP_L2TB,				"T" },
	{ CAP_L2SRB,			"B" },
	{ CAP_L2SW,				"S" },
	{ CAP_L3HOST,			"H" },
	{ CAP_IGMP,				"I" },
	{ CAP_L1,				"r" },
	{ 0,					NULL },
};

static const description_table device_capabilities[] = {
	{ CAP_L3R,				"Router" },
	{ CAP_L2TB,				"Trans-Bridge" },
	{ CAP_L2SRB,			"Source-Route-Bridge" },
	{ CAP_L2SW,				"Switch" },
	{ CAP_L3HOST,			"Host" },
	{ CAP_IGMP,				"IGMP" },
	{ CAP_L1,				"Repeater" },
	{ 0,					NULL },
};

/* CDP Frame Header */
struct cdp_frame_header {
/* Ethernet 802.3 header */
	unsigned char dst_addr[ETH_ALEN];
	unsigned char src_addr[ETH_ALEN];
	unsigned short length;
/* LLC */
	unsigned char dsap;
	unsigned char ssap;
/* LLC control */
	unsigned char control;
	unsigned char oui[3];
	unsigned short protocol_id;
} __attribute__ ((packed));

/* CDP Packet Header */
struct cdp_hdr {
	unsigned char version;			/* cdp version */
	unsigned char time_to_live;		/* cdp ttl (holdtime) */
	unsigned short checksum;		/* checksum */
};

/* CDP Frame Data */
struct cdp_field {
	unsigned short	type;
	unsigned short	length;
};

/**
 * Structura campului "Protocol Hello" e urmatoarea 
 * (dedusa dintr-un snapshot de ethereal de pe wiki-ul lor, 
 * plus some debugging):
 * 
 * - OUI [3 bytes] (0x00000c - Cisco) 
 * - Protocol ID [2 bytes] (0x0112 - Cluster Management)
 *
 *   TODO: la show cdp neighbors detail pe Cisco, urmatoarele
 * campuri sunt trantite intr-un string (cu valorile hex) de genul:
 *  value=00000000FFFFFFFF010121FF00000000000000097CCEEB00FF029A
 *
 * - Cluster Master IP [ 4 bytes ] (0.0.0.0)
 * - UNKNOWN IP? or mask? [ 4 bytes ] (255.255.255.255)
 * - Version? [ 1 byte ] (0x01)
 * - Sub Version? [ 1 byte ] (0x01)
 * - Status? [ 1 byte ] (0x21)
 * - UNKNOWN [ 1 byte ] (0xff)
 * - Cluster Commander MAC [ 6 bytes ] ( 00:00:00:00:00:00 )
 * - Switch's MAC [ 6 bytes ] ( 00:D0:BA:7A:FF:C0 )
 * - UNKNOWN [ 1 byte ] (0xff)
 * - Management VLAN [ 2 bytes ] ( 0x029a, adica 666)
 */
struct protocol_hello {
	unsigned int oui; 							/* OUI (0x00000c for Cisco) */
	unsigned short protocol_id;					/* Protocol Id (0x0112 for Cluster Management) */
	unsigned char payload[27];					/* the other fields */ 
};

/* CDP neighbor */
struct cdp_neighbor {
	struct cdp_interface *interface;		/* Interface */
	unsigned char ttl;						/* Holdtime (time to live) */
	unsigned char cdp_version;
	char device_id[64];						/* Device ID */
	unsigned char num_addr;					/* Number of decoded addresses */
	unsigned int addr[8];					/* Device addresses */
	char port_id[32];						/* Port ID */			
	unsigned char cap;						/* Capabilities */
	char software_version[255];				/* Software (Cisco IOS version) */
	char platform[32];						/* Hardware platform of the device */
	char vtp_mgmt_domain[32];				/* VTP Management Domain */
	struct protocol_hello p_hello;			/* Protocol Hello */
	unsigned char duplex;					/* Duplex */
	unsigned short native_vlan;				/* Native VLAN */
	struct  cdp_neighbor_heap_node *hnode;	/* Pointer to the associated heap node structure */
	struct list_head lh;					/* list head */
};

/* CDP interface */
struct cdp_interface {
	char name[IFNAMSIZ];					/* Name of the interface */
	int sw_sock_fd;							/* Our PF_SWITCH socket */
	sem_t n_sem;							/* semaphore used for locking on the neighbor list */
	struct list_head neighbors;				/* list of cdp neighbors (on this interface) */
	struct list_head lh;
};

/**
 * CDP global configuration settings (default values)
 */
#define CFG_DFL_VERSION 0x02
#define CFG_DFL_HOLDTIME 0xb4
#define CFG_DFL_TIMER 0x3c

/* CDP configuration parameters */
struct cdp_configuration {
	unsigned char	enabled;				/* enabled flag */
	unsigned char 	version;				/* cdp version */
	unsigned char 	holdtime;				/* cdp ttl (holdtime) in seconds */
	unsigned char	timer;					/* rate at which packets are sent (in seconds) */
	unsigned int	capabilities;			/* device capabilities */
	char		software_version[255];	/* software version information */
	char		platform[16];			/* platform information */
	unsigned char	duplex;					/* duplex */
};

struct cdp_traffic_stats {
	unsigned int	v1_in;
	unsigned int	v2_in;
	unsigned int	v1_out;
	unsigned int	v2_out;
};

/* CDP neighbor heap (used for neighbor aging mechanism) */
#define INITIAL_HEAP_SIZE 32			/* initial neighbor heap size */

#define LEFT(k)		(2*k+1)
#define RIGHT(k)	(2*k+2)
#define PARENT(k)	((int)floor((k-1)/2))
#define ROOT(heap)	(heap[0])

/* CDP neighbor heap node */
typedef struct cdp_neighbor_heap_node {
	time_t tstamp;						/* Expire timestamp */
	struct cdp_neighbor *n;				/* Pointer to the neighbor struct */
} neighbor_heap_t;

#endif

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
#include <linux/sockios.h>
#include <linux/net_switch.h>

#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>

#include "cdp_client.h"
#include "debug.h"
#include "util.h"
#include "list.h"
#include "swsock.h"
#include "switch.h"
#include "if_generic.h"

/* the pid file of the cdp daemon */
#ifdef DEBUG
#define CDPD_PID_FILE "/tmp/cdpd.pid"
#else
#define CDPD_PID_FILE "/var/run/cdpd.pid"
#endif

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
#define FIELD_TYPES {\
	{ TYPE_DEVICE_ID,		"Device ID" },\
	{ TYPE_ADDRESS,			"Addresses" },\
	{ TYPE_PORT_ID,			"Port ID" },\
	{ TYPE_CAPABILITIES,	"Capabilities" },\
	{ TYPE_IOS_VERSION,		"Software version" },\
	{ TYPE_PLATFORM,		"Platform" },\
	{ TYPE_IP_PREFIX,		"IP Prefix/Gateway" },\
	{ TYPE_PROTOCOL_HELLO,	"Protocol Hello" },\
	{ TYPE_VTP_MGMT_DOMAIN, "VTP Management Domain" },\
	{ TYPE_NATIVE_VLAN,		"Native VLAN" },\
	{ TYPE_DUPLEX,			"Duplex" },\
	{ TYPE_VOIP_VLAN_REPLY, "VoIP VLAN Reply" },\
	{ TYPE_VOIP_VLAN_QUERY,	"VoIP VLAN Query" },\
	{ TYPE_MTU,				"MTU" },\
	{ TYPE_TRUST_BITMAP,	"Trust Bitmap" },\
	{ TYPE_UNTRUSTED_COS,	"Untrusted Port CoS" },\
	{ TYPE_SYSTEM_NAME,		"System Name" },\
	{ TYPE_SYSTEM_OID,		"System Object ID" },\
	{ TYPE_MGMT_ADDR, 		"Management Address" },\
	{ TYPE_LOCATION,		"Location" },\
	{ 0,					NULL },\
}

/* possible values for the protocol type in address fields */
#define PROTO_TYPE_NLPID		0x01
#define PROTO_TYPE_802_2		0x02

/* possible values for the protocol value in address fields */
#define PROTO_ISO_CLNS 			0x81  	/* protocol type 3D 1 */
#define PROTO_IP				0xCC  	/* protocol type 3D 1 */
/* for 802.2 the protocol value is 0xAAAA 0300 0000 followed by one of: */
#define PROTO_IPV6				0x0800 	/* protocol type 3D 2 */
#define PROTO_DECNET_PHASE_IV 	0x6003 	/* protocol type 3D 2 */
#define PROTO_APPLETALK			0x809B 	/* protocol type 3D 2 */
#define PROTO_NOVELL_IPX 		0x8137 	/* protocol type 3D 2 */
#define PROTO_BANYAN_VINES		0x80C4 	/* protocol type 3D 2 */
#define PROTO_XNS				0x0600 	/* protocol type 3D 2 */
#define PROTO_APOLLO_DOMAIN		0x8019 	/* protocol type 3D 2 */

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

/* CDP Neighbor */
struct cdp_neighbor {
	struct cdp_interface *interface;		/* Interface */
	struct cdp_neighbor_info info;			/* Information about the neighbor */
	struct cdp_neighbor_heap_node *hnode;	/* Pointer to the associated heap node structure */
	struct list_head lh;					/* list head */
};

/* CDP interface */
struct cdp_interface {
	int if_index;							/* Interface index */
	int sw_sock_fd;							/* Our PF_SWITCH socket */
	sem_t n_sem;							/* semaphore used for locking on the neighbor list */
	struct list_head neighbors;				/* list of cdpd neighbors (on this interface) */
	struct list_head lh;					/* list of cdp interfaces */
};

/* CDP neighbor heap (used for neighbor aging mechanism) */
#define INITIAL_HEAP_SIZE 32

/* CDP neighbor heap node */
typedef struct cdp_neighbor_heap_node {
	time_t tstamp;			/* Expire timestamp */
	struct cdp_neighbor *n;	/* Pointer to the cdpd neighbor struct */
} neighbor_heap_t;

void cdpd_register_interface(int if_index);
void cdpd_unregister_interface(int if_index);
int  cdpd_get_interface_status(int if_index);
void sift_up(neighbor_heap_t *, int);
void sift_down(neighbor_heap_t *, int, int);
void *cdp_send_loop(void *);
void *cdp_ipc_listen(void *);
void *cdp_clean_loop(void *);

#endif

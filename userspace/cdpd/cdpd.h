#ifndef _CDPD_H
#define _CDPD_H

#include "list.h"

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

/* constants for device capabilities (capability masks) */
#define CAP_L3R				0x01	/* device is a layer 3 router */
#define CAP_L2TB			0x02	/* device is a layer 2 transparent bridge */
#define CAP_L2SRB			0x04	/* device is a layer 2 source-route bridge */
#define CAP_L2SW			0x08	/* device is a layer 2 switch (non-spanning tree) */
#define CAP_L3HOST			0x10	/* device is a layer 3 (non routing) host */
#define CAP_IGMP			0x20	/* device is IGMP capable */
#define CAP_L1				0x40	/* device is a layer 1 repeater */

/* file from which we fetch the system interfaces */
/* FIXME: constant already defined in cli/if.h ... */
#define PROCNETDEV_PATH "/proc/net/dev"

/* pcap filter expression for cdp */
#define PCAP_CDP_FILTER "ether multicast and ether[20:2] = 0x2000"

/* CDP Frame Header */
struct cdp_frame_hdr {
	u_int8_t	version;
	u_int8_t	time_to_live;
	u_int16_t	checksum;
};

/* CDP Frame Data */
struct cdp_frame_data {
	u_int16_t	type;
	u_int16_t	length;
};

/* Registered interface struct */
struct cdp_registered_if {
	char name[IFNAMSIZ];
	bpf_u_int32 addr, netmask;
	pcap_t *pcap;
	struct list_head lh;
};

#endif

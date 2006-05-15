#ifndef _CDPD_H
#define _CDPD_H

#include "list.h"

/* file from which we fetch the system interfaces */
/* FIXME: constant already defined in cli/if.h ... */
#define PROCNETDEV_PATH "/proc/net/dev"

/* pcap filter expression for cdp */
#define PCAP_CDP_FILTER "ether multicast and ether[20:2] = 0x2000"


/* integer to alphanumeric mapping */
typedef struct {
	const u_int16_t value;
	const u_char *description;
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
static const description_table device_capabilities[] = {
	{ CAP_L3R,				"Layer 3 Router" },
	{ CAP_L2TB,				"Layer 2 Transparent Bridge" },
	{ CAP_L2SRB,			"Layer 2 Source-Route Bridge" },
	{ CAP_L2SW,				"Layer 2 Switch (non-stp)" },
	{ CAP_L3HOST,			"Host" },
	{ CAP_IGMP,				"Device is IGMP capable" },
	{ CAP_L1,				"Layer 1 Repeater" },
	{ 0,					NULL },
};

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

/* CDP neighbor */
struct cdp_neighbor {
	char *device_id;			/* Device ID */
	/* XXX - Addresses */
	char *port_id;				/* Port ID */			
	u_char capabilities;		/* Capabilities */
	char *ios_version;			/* Cisco IOS Version */
	char *platform;				/* Hardware platform of the device */
	/* XXX - ip prefix */
	/* XXX - the other fields ... */
};

/* CDP interface */
struct cdp_interface {
	char name[IFNAMSIZ];		/* Name of the interface */
	bpf_u_int32 addr, netmask;	/* IP/netmask */
	pcap_t *pcap;				/* pcap structure */
	struct list_head neighbors;	/* list of cdp neighbors (on this interface) */
	struct list_head lh;
};

#endif

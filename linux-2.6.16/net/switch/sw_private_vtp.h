#ifndef _SW_PRIVATE_VTP_H
#define _SW_PRIVATE_VTP_H 


#define VTP_DEFAULT_VERSION	0X01

#define VTP_SUMM_ADVERT		0x01
#define VTP_SUBSET_ADVERT	0x02
#define VTP_REQUEST		0x03
#define VTP_JOIN		0x04

#define VTP_DOMAIN_SIZE		32
#define VTP_MAX_PASSWORD_SIZE	64
#define VTP_MD5_SIZE		16

#define VTP_LLC_HEADER_LEN	3
#define VTP_SNAP_HEADER_LEN	5

#define VTP_SUMMARY_DELAY	(5*HZ) //ARE VALOARE 300
#define DEFAULT_MTU_SIZE	1500

#define VLAN_TYPE_ETHERNET	0x01
#define VLAN_TYPE_FDDI		0x02
#define VLAN_TYPE_TRCRF		0x03
#define VLAN_TYPE_FDDI_NET	0x04
#define VLAN_TYPE_TRBRF		0x05

#define VLAN_TYPE_FDDI_NO	1002
#define VLAN_TYPE_TRCRF_NO	1003
#define VLAN_TYPE_FDDI_NET_NO	1004
#define VLAN_TYPE_TRBRF_NO	1005

#define ETHERNET_MTU		1500
#define FDDI_MTU		1500
#define TRCRF_MTU		4472
#define FDDI_NET_MTU		1500
#define TRBRF_MTU		4472

#define PROCESS_CONTEXT		0X01
#define INTERRUPT_CONTEXT	0X02

#define BASE_802DOT10_INDEX	0x186A0

#define VTP_VLAN_MASK_LG        126




static const unsigned char vtp_multicast_mac[ETH_ALEN] = { 0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC };
static const unsigned char vtp_llc_header[VTP_LLC_HEADER_LEN] = { 0xAA, 0xAA, 0x03 };
static const unsigned char vtp_snap_header[VTP_SNAP_HEADER_LEN] = { 0x00, 0x00, 0x0C, 0x20, 0x03 };


struct sw_vtp_vars
{
	spinlock_t lock;
	atomic_t  vtp_enabled;	
	
	u8  password_enabled;
	char password[VTP_MAX_PASSWORD_SIZE];
	u8  md5[VTP_MD5_SIZE];
	
	u8  dom_len;
	u8  domain[VTP_DOMAIN_SIZE];
	
	u8  mode;
	u8  version;
	u32 revision;
	
	u8  last_updater[4];
	u8  last_update[VTP_TIMESTAMP_SIZE+1];
	u16 last_subset;
	u16 followers;
	
	u8  pruning_enabled;
	
	struct list_head subsets;
	int subset_list_size;

	struct timer_list	summary_timer;
	struct timer_list	request_timer;
};

struct sw_port_vtp_vars
{
	u8 active_vlans[VTP_VLAN_MASK_LG];
};

struct vlan_info 
{
	/* Linking with other infos in a list. */
	struct list_head lh;

	u8  len;
	u8  status;
	u8  type;
	u8  name_len;
	u16 id;
	u16 mtu;
	u32 dot10;
	char name[SW_MAX_VLAN_NAME+1];
};

struct vtp_summary 
{
	u8  version;
	u8  code;
	u8  followers; 
	u8  dom_len;
	u8  domain[VTP_DOMAIN_SIZE];
	u32 revision;
	u32 updater;
	u8  timestamp[VTP_TIMESTAMP_SIZE];
	u8  md5[VTP_MD5_SIZE];
};

struct vtp_subset 
{
	u16 length;
	u8  version;
	u8  code;
	u8  seq;
	u8  dom_len;
	u8  domain[VTP_DOMAIN_SIZE];
	u32 revision;
	
	/* List of vlan info. */	
	struct list_head infos;

	/* List of subsest. */
	struct list_head subsets;

};

struct vtp_request 
{
	u8  version;
	u8  code;
	u8  reserved;
	u8  dom_len;
	u8  domain[VTP_DOMAIN_SIZE];
	u16 start_val;
};

struct vtp_join 
{
	u8  version;
	u8  code;
	u8  reserved;
	u8  dom_len;
	u8  domain[VTP_DOMAIN_SIZE]; 
	u32 vlan;
	u8  active_vlans[126];
};


#endif 

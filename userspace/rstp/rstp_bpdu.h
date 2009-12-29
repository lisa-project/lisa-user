#ifndef _RSTP_BPDU_H__
#define _RSTP_BPDU_H__

#define BPDU_TCN		0x80
#define BPDU_CONFIG             0x00
#define BPDU_RSTP               0x02

#define TOPOLOGY_CHANGE_BIT     0x01
#define PROPOSAL_BIT            0x02
#define PORT_ROLE_MASK          0x0C
#define LEARN_BIT               0x10
#define FORWARD_BIT             0x20
#define AGREEMENT_BIT           0x40
#define TOPOLOGY_CHANGE_ACK_BIT 0x80

#define RSTP_DISABLED		0x00

#define ROLE_DISABLED		0x00

#define ROLE_UNKNOWN		0x00
#define ROLE_ALTERNATE_BACKUP	0x04
#define ROLE_ROOT		0x08
#define ROLE_DESIGNATED		0x0C
#define ROLE_ALTERNATE		0x05
#define ROLE_BACKUP		0x06

struct mac_header_t {
	unsigned char dst_mac[6];
	unsigned char src_mac[6];
} __attribute__ ((packed));

struct eth_header_t {
	unsigned char len[2];
	unsigned char dsap;
	unsigned char ssap;
	unsigned char llc;
} __attribute__ ((packed));

struct bpdu_header_t {
	unsigned char protocol[2];
	unsigned char version;
	unsigned char bpdu_type;
} __attribute__ ((packed));

struct bpdu_body_t {
	unsigned char flags;
	unsigned char root_id[8];
	unsigned char root_path_cost[4];
	unsigned char bridge_id[8];
	unsigned char port_id[2];
	unsigned char message_age[2];
	unsigned char max_age[2];
	unsigned char hello_time[2];
	unsigned char forward_delay[2];
} __attribute__ ((packed));

struct bpdu_t {
	struct mac_header_t mac; //12
	struct eth_header_t eth; //5
	struct bpdu_header_t hdr; //4
	struct bpdu_body_t body; //31
	unsigned char ver_1_len; //2
} __attribute__ ((packed));

#endif

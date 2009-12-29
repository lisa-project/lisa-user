#ifndef _RSTP_BPDU_H__
#define _RSTP_BPDU_H__

#define MIN_BPDU                7
#define BPDU_L_SAP              0x42
#define LLC_UI                  0x03
#define BPDU_PROTOCOL_ID        0x0000
#define BPDU_VERSION_ID         0x00
#define BPDU_VERSION_RAPID_ID   0x02

#define BPDU_TOPO_CHANGE_TYPE   0x80
#define BPDU_CONFIG_TYPE        0x00
#define BPDU_RSTP               0x02

#define TOLPLOGY_CHANGE_BIT     0x01
#define PROPOSAL_BIT            0x02
#define PORT_ROLE_OFFS          2   /* 0x04 & 0x08 */
#define PORT_ROLE_MASK          (0x03 << PORT_ROLE_OFFS)
#define LEARN_BIT               0x10
#define FORWARD_BIT             0x20
#define AGREEMENT_BIT           0x40
#define TOLPLOGY_CHANGE_ACK_BIT 0x80

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

struct stp_bpdu_t {
	struct mac_header_t mac;
	struct eth_header_t  eth;
	struct bpdu_header_t hdr;
	struct bpdu_body_t body;
	unsigned char ver_1_len[2];
} __attribute__ ((packed));

#endif

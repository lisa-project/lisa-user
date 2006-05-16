#include <sys/types.h>
#include <sys/ioctl.h>

#include <linux/net_switch.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "cdpd.h"
#include "debug.h"

LIST_HEAD(registered_interfaces);
LIST_HEAD(deregistered_interfaces);

/* get the description string from a description table */
static const u_char *get_description(u_int16_t val, const description_table *table) {
	u_int16_t i = 0;

	while (table[i].description) {
		if (table[i].value == val)
			break;
		i++;
	}

	return table[i].description;
}

/**
 * add an interface to the list of cdp-enabled
 * interfaces.
 */
void register_cdp_interface(char *ifname) {
	int fd;
	struct cdp_interface *entry, *tmp;
	struct bpf_program filter;
	char pcap_err[PCAP_ERRBUF_SIZE];


	dbg("Enabling cdp on '%s'\n", ifname);

	/* search for the interface in the deregistered_interfaces list */
	list_for_each_entry_safe(entry, tmp, &deregistered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			list_del(&entry->lh);
			list_add_tail(&entry->lh, &registered_interfaces);
			return;
		}

	/* if not found, we must allocate the structure and do the 
	 proper intialization */
	entry = (struct cdp_interface *) malloc(sizeof(struct cdp_interface));
	assert(entry);
	strncpy(entry->name, ifname, IFNAMSIZ);

	pcap_lookupnet(entry->name, &entry->addr, &entry->netmask, pcap_err);
	entry->pcap = pcap_open_live(entry->name, 65535, 1, 0, pcap_err);
	if (!entry->pcap) {
		fprintf(stderr, "Could not open %s: %s\n", entry->name, pcap_err);
		exit(1);
	}
	pcap_compile(entry->pcap, &filter, PCAP_CDP_FILTER, 0, entry->addr);
	pcap_setfilter(entry->pcap, &filter);
	pcap_freecode(&filter);

	fd = pcap_fileno(entry->pcap);
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	/* add the entry to the registered_interfaces list */
	list_add_tail(&entry->lh, &registered_interfaces);
}

/**
 * remove an interface from the list of cdp-enabled
 * interfaces.
 */
static void unregister_cdp_interface(char *ifname) {
	struct cdp_interface *entry, *tmp;

	/* move the entry to the deregistrated_interfaces list */
	dbg("Disabling cdp on '%s'\n", ifname);
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			list_del(&entry->lh);
			list_add_tail(&entry->lh, &deregistered_interfaces);
		}
}

/* fetch the interface name from a /proc/net/dev line */
static void fetch_if_name(char *name, char *buf) {
	int i = 0;

	while (isspace(*buf))
		buf++;
	while (*buf) {
		if (isspace(*buf))
			break;
		if (*buf == ':')
			break;
		name[i++] = *buf++;
		if (i >= IFNAMSIZ-1)
			break;
	}
	name[i] = '\0';
}

/**
 * parse the system interfaces from /proc/net/dev
 * and enable cdp on those who are in the switch. 
 */
static void do_initial_register() {
	int sockfd;
	FILE *f;
	char buf[512], name[IFNAMSIZ];
	struct ifreq ifr;

	if (!(f = fopen(PROCNETDEV_PATH, "r"))) {
		perror("fopen");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		fclose(f);
		exit(1);
	}

	/* skip 2 lines */
	fgets(buf, sizeof(buf), f);
	fgets(buf, sizeof(buf), f);

	while (fgets(buf, sizeof(buf), f)) {
		fetch_if_name(name, buf);
		/* TODO: register interface only if the interface is added 
		 * to the switch (check with ioctl), and interface is up */
		strncpy(ifr.ifr_name, name, IFNAMSIZ);
		/* get interface flags */
		if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
			perror("ioctl");
			close(sockfd);
			fclose(f);
			exit(1);
		}
		if (!(ifr.ifr_flags & IFF_UP))
			continue;
		register_cdp_interface(name);
	}
	fclose(f);
	close(sockfd);
}

/**
 * alpha-numeric field.
 */
void decode_alpha(u_char *field) {
	dbg("\t\tfield value: '%s'\n", field);
}

void decode_ipv4_addr(u_int32_t addr) {
	dbg("\t\t\tIP addr: %d.%d.%d.%d\n", addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
}

/**
 * Address Field Structure:
 * 
 * number_of_addresses
 * Address_1
 * Address_2
 * ...
 * Address_k
 *
 * where Address fields have the structure:
 *
 * Protocol Type (1 byte) [1 - NLPID format, 2 - 802.2 format]
 * Length (1 byte)	[ 1 for protocol type 1, 3 or 8 for protocol type 2, depending
 * 		on whether SNAP is used]
 * Protocol Value (variable) [ for possible values see constants in cdpd.h ]
 * Addr Len (2 bytes) [ length of the address field in bytes ]
 * Address (variable) [ address of the interface, or address of the system if addresses
 * 		are not assigned to the interface ]
 */
void decode_address(u_char *field, u_int32_t len) {
	u_int32_t i, number, addr;

	number = ntohl(*((u_int32_t *)field));
	field += sizeof(number);
	dbg("\t\tnumber of addresses: %d\n", number);
	for (i = 0; i<number; i++) {
		u_char protocol_type = field[0];
		u_char length = field[1]; 
		dbg("\t\t\t%d) ptype: %d, length: %d\n", i+1, protocol_type, length);
		if (protocol_type != 1 || length != 1) {
			dbg("\t\t\tUnsupported protocol type.");
			field += length;
			continue;
		}
		u_char pvalue = field[2]; 
		dbg("\t\t\tProtocol is 0x%hx, %s.\n", pvalue, get_description(pvalue, proto_values));
		if (pvalue != PROTO_IP) {
			dbg("\t\t\tOnly IPv4 supported. got pvalue: 0x%x.\n", pvalue);
			field += length;
			continue;
		}
		addr = ntohl(*((u_int32_t *) (field + 5)));
		decode_ipv4_addr(addr);
		field += length;
	}
}

void decode_capabilities(u_char *field, u_int32_t len) {
	u_int16_t i = 0;
	u_int32_t cap = ntohl(*((u_int32_t *)field));

	dbg("\t\t\tcapa: 0x%x\n", cap);

	while (device_capabilities[i].description) {
		if (cap & device_capabilities[i].value) 
			dbg("\t\t\t[%s]\n", device_capabilities[i].description);
		i++;
	}
}

void decode_duplex(u_char *field, u_int32_t len) {
	dbg("\t\t\tvalue: 0x%x\n", field[0]);
}

void decode_native_vlan(u_char *field, u_int32_t len) {
	u_int16_t nv = ntohs(*((u_int16_t *) field));
	dbg("\t\t\tvalue: %d\n", nv);
}

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
void decode_protocol_hello(u_char *field, u_int32_t len) {
	u_int32_t i;

	dbg("\t\t\t");
	for (i=0; i<len; i++) {
		dbg("0x%02x ", field[i]);
	}
	dbg("\n");
}

void decode_field(int type, int len, u_char *field) {
	dbg("\t[%s], length: %d\n", get_description(type, field_types), len);
	switch (type) {
	case TYPE_DEVICE_ID:
	case TYPE_PORT_ID:
	case TYPE_IOS_VERSION:
	case TYPE_PLATFORM:
	case TYPE_VTP_MGMT_DOMAIN:
		decode_alpha(field);
		break;
	case TYPE_ADDRESS:
		decode_address(field, len);
		break;
	case TYPE_CAPABILITIES:
		decode_capabilities(field, len);
		break;
	case TYPE_PROTOCOL_HELLO:
		decode_protocol_hello(field, len);
		break;
	case TYPE_DUPLEX:
		decode_duplex(field, len);
		break;
	case TYPE_NATIVE_VLAN:
		decode_native_vlan(field, len);
		break;
	}
}

/* packet introspection function */
void dissect_packet(struct cdp_interface *entry, struct pcap_pkthdr *header, 
		u_char *packet) {
	struct cdp_frame_hdr *cdp_hdr;
	struct cdp_frame_data *cdp_field;
	u_int32_t packet_length, consumed, field_type, field_len;

	dbg("Received CDP packet on %s", entry->name);

	cdp_hdr = (struct cdp_frame_hdr *) (((u_char *)packet) + 22);
	packet_length = (header->len - 22) - sizeof(struct cdp_frame_hdr);
	dbg("\tpacket length: %d\n", packet_length);
	cdp_field = (struct cdp_frame_data *) ((((u_char *)packet) + 22) + sizeof(struct cdp_frame_hdr));

	consumed = 0;
	while (consumed < packet_length) {
		field_type = ntohs(cdp_field->type);
		field_len = ntohs(cdp_field->length);
		decode_field(field_type, field_len-sizeof(struct cdp_frame_data), 
				((u_char *)cdp_field)+sizeof(struct cdp_frame_data));
		consumed += field_len;	
		cdp_field = (struct cdp_frame_data *) (((u_char *)cdp_field) + field_len);
	}
	dbg("\n\n");
}

int main(int argc, char *argv[]) {
	struct cdp_interface *entry, *tmp;
	int fd, maxfd;
	fd_set rdfs;
	struct pcap_pkthdr header;
	u_char *packet;

	do_initial_register();
	/* hey, no interface in the switch?? */
	assert(!list_empty(&registered_interfaces));

	/* the main loop in which we capture the cdp frames */
	for (;;) {
		FD_ZERO(&rdfs);
		maxfd = -1;
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			FD_SET((fd = pcap_fileno(entry->pcap)), &rdfs);
			if (fd > maxfd)
				maxfd = fd;
		}
		select(maxfd+1, &rdfs, 0, 0, 0);
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			packet = (u_char *) pcap_next(entry->pcap, &header);
			if (packet)
				dissect_packet(entry, &header, packet);
		}
	}

	return 0;
}

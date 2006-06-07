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

	INIT_LIST_HEAD(&entry->neighbors);

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

static struct cdp_neighbor *find_by_device_id(struct cdp_interface *entry, u_char *device_id) {
	struct cdp_neighbor *n, *tmp;

	list_for_each_entry_safe(n, tmp, &(entry->neighbors), lh)
		if (!strncmp(n->device_id, device_id, sizeof(n->device_id)))
			return n;
	return NULL;
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

static void print_ipv4_addr(u_int32_t addr) {
	dbg("%d.%d.%d.%d", addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
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
static void decode_address(u_char *field, u_int32_t len, struct cdp_neighbor *n) {
	u_int32_t i, number, offset;

	number = ntohl(*((u_int32_t *)field));
	field += sizeof(number);

	if (number > sizeof(n->addr))
		number = sizeof(n->addr);

	for (i = 0, offset = 0; i<number; i++) {
		u_char protocol_type = field[0];
		u_char length = field[1]; 
		if (protocol_type != 1 || length != 1) {
			dbg("\t\t\tUnsupported protocol type.");
			field += length;
			continue;
		}
		u_char pvalue = field[2]; 
		if (pvalue != PROTO_IP) {
			dbg("\t\t\tOnly IPv4 supported. got pvalue: 0x%x.\n", pvalue);
			field += length;
			continue;
		}
		n->addr[offset++] = ntohl(*((u_int32_t *) (field + 5)));
		field += length;
	}
	n->num_addr = offset;
}

static void print_capabilities(u_char cap) {
	u_int16_t i = 0;

	while (device_capabilities[i].description) {
		if (cap & device_capabilities[i].value) 
			dbg("\t\t[%s]\n", device_capabilities[i].description);
		i++;
	}
}

static void decode_protocol_hello(u_char *field, u_int32_t len, struct protocol_hello *h) {
	h->oui = ntohl(*((u_int32_t *)field)) >> 8;
	h->protocol_id = ntohs(*((u_int16_t *) (field + 3)));
	memcpy(h->payload, (field + 5), sizeof(h->payload));
}

static void copy_alpha_field(u_char *dest, u_char *src, size_t size) {
	strncpy(dest, src, size);
	dest[size-1] = '\0';
}

static void decode_field(struct cdp_neighbor **ne, int type, int len, u_char *field) {
	struct cdp_neighbor* neighbor = *ne;
	struct cdp_neighbor* n = NULL;
	switch (type) {
	case TYPE_DEVICE_ID:
		n = find_by_device_id(neighbor->interface, field);
		if (n) {
			free(neighbor);
			dbg("\t\tneighbor %s already registered.\n", field);
			neighbor = n;
			*ne = neighbor;
		}
		else {
			/* a new neighbor was found */
			dbg("\t\tnew neighbor: %s.\n", field);
			copy_alpha_field(neighbor->device_id, field, sizeof(neighbor->device_id));
			list_add_tail(&neighbor->lh, &neighbor->interface->neighbors);
		}
		break;
	case TYPE_PORT_ID:
		copy_alpha_field(neighbor->port_id, field, sizeof(neighbor->port_id));
		break;
	case TYPE_IOS_VERSION:
		copy_alpha_field(neighbor->software_version, field, sizeof(neighbor->software_version));
		break;
	case TYPE_PLATFORM:
		copy_alpha_field(neighbor->platform, field, sizeof(neighbor->platform));
		break;
	case TYPE_VTP_MGMT_DOMAIN:
		copy_alpha_field(neighbor->vtp_mgmt_domain, field, sizeof(neighbor->vtp_mgmt_domain));
		break;
	case TYPE_ADDRESS:
		decode_address(field, len, neighbor);
		break;
	case TYPE_CAPABILITIES:
		neighbor->cap = ntohl(*((u_int32_t *)field));
		break;
	case TYPE_PROTOCOL_HELLO:
		decode_protocol_hello(field, len, &neighbor->p_hello);
		break;
	case TYPE_DUPLEX:
		neighbor->duplex = field[0];
		break;
	case TYPE_NATIVE_VLAN:
		neighbor->native_vlan = ntohs(*((u_int16_t *) field));
		break;
	default:
		dbg("\t\twe don't decode [%s].\n", get_description(type, field_types));
	}
}

/* packet introspection function */
/**
 * Trebuie sa existe un hold time pentru o inregistrare de neighbor.
 * Daca holdtime expira -> zboara inregistrarea.
 */
static void dissect_packet(struct pcap_pkthdr *header, u_char *packet, struct cdp_neighbor **neighbor) {
	struct cdp_frame_hdr *cdp_hdr;
	struct cdp_frame_data *cdp_field;
	u_int32_t packet_length, consumed, field_type, field_len;

	dbg("Received CDP packet on %s", (*neighbor)->interface->name);

	cdp_hdr = (struct cdp_frame_hdr *) (((u_char *)packet) + 22);
	packet_length = (header->len - 22) - sizeof(struct cdp_frame_hdr);
	dbg("\tpacket length: %d\n", packet_length);
	cdp_field = (struct cdp_frame_data *) ((((u_char *)packet) + 22) + sizeof(struct cdp_frame_hdr));

	consumed = 0;
	while (consumed < packet_length) {
		field_type = ntohs(cdp_field->type);
		field_len = ntohs(cdp_field->length);
		decode_field(neighbor, field_type, field_len-sizeof(struct cdp_frame_data), 
				((u_char *)cdp_field)+sizeof(struct cdp_frame_data));
		consumed += field_len;	
		cdp_field = (struct cdp_frame_data *) (((u_char *)cdp_field) + field_len);
	}
	dbg("\n\n");
}

/**
 * Display the contents of the cdp_neighbor structure.
 */
static void print_cdp_neighbor(struct cdp_neighbor *n) {
	int i;
	dbg("CDP neighbor [%p] on %s\n", n, n->interface->name);
	dbg("--------------------------------------\n");
	dbg("\t[%s]: '%s'\n", get_description(TYPE_DEVICE_ID, field_types), n->device_id);
	dbg("\t[%s] (%d):\n", get_description(TYPE_ADDRESS, field_types), n->num_addr);
	for (i=0; i<n->num_addr; i++) {
		dbg("\t\t\t");
		print_ipv4_addr(n->addr[i]);
		dbg("\n");
	}
	dbg("\t[%s]: '%s'\n", get_description(TYPE_PORT_ID, field_types), n->port_id);
	dbg("\t[%s]:\n", get_description(TYPE_CAPABILITIES, field_types));
	print_capabilities(n->cap);
	dbg("\t[%s]: '%s'\n", get_description(TYPE_IOS_VERSION, field_types), n->software_version);
	dbg("\t[%s]: '%s'\n", get_description(TYPE_PLATFORM, field_types), n->platform);
	dbg("\t[%s]: '%s'\n", get_description(TYPE_VTP_MGMT_DOMAIN, field_types), n->vtp_mgmt_domain);
	dbg("\t[%s]:\n", get_description(TYPE_PROTOCOL_HELLO, field_types));
	dbg("\t\tOUI: 0x%02X%02X%02X\n", n->p_hello.oui >> 16, (n->p_hello.oui >> 8) & 0xFF,
			n->p_hello.oui &0xFF);
	dbg("\t\tProtocol ID: 0x%02X%02X\n", n->p_hello.protocol_id >> 8, 
			n->p_hello.protocol_id & 0xFF);
	dbg("\t\tpayload len: %d\n", sizeof(n->p_hello.payload));
	dbg("\t\tvalue: ");
	for (i=0; i<sizeof(n->p_hello.payload); i++) {
		dbg("%02X", n->p_hello.payload[i]);
	}
	dbg("\n");
	dbg("\t[%s]: %d\n", get_description(TYPE_DUPLEX, field_types), n->duplex);
	dbg("\t[%s]: %d\n", get_description(TYPE_NATIVE_VLAN, field_types), n->native_vlan);
	dbg("\n");
}

int main(int argc, char *argv[]) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *neighbor;
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
			if (packet) {
				neighbor = (struct cdp_neighbor *) malloc(sizeof(struct cdp_neighbor));
				neighbor->interface = entry;
				dissect_packet(&header, packet, &neighbor);
				print_cdp_neighbor(neighbor);
			}
		}
	}

	return 0;
}

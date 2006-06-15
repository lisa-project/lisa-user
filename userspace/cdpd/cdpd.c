#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <linux/net_switch.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <pcap.h>
#include <libnet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "cdpd.h"
#include "debug.h"

LIST_HEAD(registered_interfaces);
LIST_HEAD(deregistered_interfaces);

/* data buffer for cdp frame building */
static unsigned char data[65535];
/* cdp configuration parameters (version, ttl, timer) */
static struct cdp_configuration cfg;

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
	int fd, addr;
	struct cdp_interface *entry, *tmp;
	struct bpf_program filter;
	char err[PCAP_ERRBUF_SIZE];
	char buf[128]; 


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


	/* libnet initialization */
	entry->llink = libnet_init(LIBNET_LINK, entry->name, err);
	if (!entry->llink) {
		fprintf(stderr, "Libnet failed opening %s: %s\n", entry->name, err);
		exit(1);
	}
	entry->hwaddr = libnet_get_hwaddr(entry->llink);
	if (!entry->hwaddr) {
		fprintf(stderr, "Libnet failed getting hw addr of %s: %s\n", entry->name, err);
		exit(1);
	}
	entry->addr = libnet_get_ipaddr4(entry->llink);

	/* pcap initialization */
	pcap_lookupnet(entry->name, &addr, &entry->netmask, err);
	entry->pcap = pcap_open_live(entry->name, 65535, 1, 0, err);
	if (!entry->pcap) {
		fprintf(stderr, "Pcap failed opening %s: %s\n", entry->name, err);
		exit(1);
	}
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), PCAP_CDP_FILTER, entry->hwaddr->ether_addr_octet[0], entry->hwaddr->ether_addr_octet[1], 
			entry->hwaddr->ether_addr_octet[2], entry->hwaddr->ether_addr_octet[3], entry->hwaddr->ether_addr_octet[4],
			entry->hwaddr->ether_addr_octet[5]);
	buf[sizeof(buf)-1] = '\0';
	dbg("Pcap expresion[%s]: %s, %p, %p, %p\n", entry->name, buf, entry, entry->hwaddr, entry->hwaddr->ether_addr_octet);
	pcap_compile(entry->pcap, &filter, buf, 0, entry->addr);
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

/**
 * Identify a cdp neighbor in the neighbor list by its
 * device_id.
 */
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
		// quick hack, please remove
		// only interfaces registered in the switch...
		if (!strncmp(name, "lo", strlen("lo")))
			continue;
		register_cdp_interface(name);
	}
	fclose(f);
	close(sockfd);
}

/* Default CDP configuration */
static void do_initial_cfg() {
	struct utsname u_name;

	cfg.version = 0x02;				/* CDPv2*/
	cfg.holdtime = 0xb4;			/* 180 seconds */
	cfg.timer = 0x3c;				/* 60 seconds */
	cfg.capabilities = CAP_L2SW;	/* advertise as a layer 2 (non-STP) switch */
	cfg.duplex = 0x01;
	/* get uname information and set software version and platform */
	uname(&u_name);
	memset(cfg.software_version, 0, sizeof(cfg.software_version));
	snprintf(cfg.software_version, sizeof(cfg.software_version), 
			"Linux %s, %s.", u_name.release, u_name.version);
	cfg.software_version[sizeof(cfg.software_version)-1] = '\0';
	memset(cfg.platform, 0, sizeof(cfg.platform));
	snprintf(cfg.platform, sizeof(cfg.platform), "%s", u_name.machine);
	cfg.platform[sizeof(cfg.platform)-1] = '\0';
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
	struct cdp_hdr *hdr;
	struct cdp_field *field;
	u_int32_t packet_length, consumed, field_type, field_len;

	dbg("Received CDP packet on %s", (*neighbor)->interface->name);

	hdr = (struct cdp_hdr *) (((u_char *)packet) + 22);
	packet_length = (header->len - 22) - sizeof(struct cdp_hdr);
	dbg("\tpacket length: %d\n", packet_length);
	field = (struct cdp_field *) ((((u_char *)packet) + 22) + sizeof(struct cdp_hdr));

	consumed = 0;
	while (consumed < packet_length) {
		field_type = ntohs(field->type);
		field_len = ntohs(field->length);
		decode_field(neighbor, field_type, field_len-sizeof(struct cdp_field), 
				((u_char *)field)+sizeof(struct cdp_field));
		consumed += field_len;	
		field = (struct cdp_field *) (((u_char *)field) + field_len);
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

/**
 *  Functions for building and sending cdp frames
 */
 
/**
 * Fill in the cdp frame header fields.
 */
static int cdp_frame_init(u_char *buffer, int len, struct libnet_ether_addr *hw_addr) {
	memset(buffer, 0, len);
	struct cdp_frame_header *fhdr;
	struct cdp_hdr *phdr;

	fhdr = (struct cdp_frame_header *)buffer;
	/* dst mac is multicast (01:00:0c:cc:cc:cc) */
	fhdr->dst_addr[0] = 0x01;
	fhdr->dst_addr[1] = 0x00;
	fhdr->dst_addr[2] = 0x0c;
	fhdr->dst_addr[3] = fhdr->dst_addr[4] = fhdr->dst_addr[5] = 0xcc;
	/* src mac is our mac address */
	memcpy(fhdr->src_addr, hw_addr->ether_addr_octet, ETH_ALEN);

	/* DSAP & SSAP addresses are 0xaa (SNAP) */
	fhdr->dsap = fhdr->ssap = 0xaa;
	fhdr->control = 0x03;
	/* OUI for Cisco */
	fhdr->oui[2] = 0x0c;
	/* CDP protocol id: 0x2000 */
	fhdr->protocol_id = htons(0x2000);

	/* Now the CDP packet header */
	phdr = (struct cdp_hdr *) (buffer + sizeof(struct cdp_frame_header));
	/* CDP version */
	phdr->version = cfg.version;
	/* CDP holdtime */
	phdr->time_to_live = cfg.holdtime;
	/* Checksum will be calculated later */
	phdr->checksum = 0x00;

	return sizeof(struct cdp_frame_header) + sizeof(struct cdp_hdr);
}

/**
 * Add the device id field.
 */
static int cdp_add_device_id(u_char *buffer) {
	u_char hostname[MAX_HOSTNAME];
	struct cdp_field *field;

	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname)-1] = '\0';

	field = (struct cdp_field *) buffer;
	field->type = htons(TYPE_DEVICE_ID);
	field->length = htons(strlen(hostname) + sizeof(struct cdp_field));

	memcpy(buffer+sizeof(struct cdp_field), hostname, strlen(hostname));

	return sizeof(struct cdp_field) + strlen(hostname);
}

/**
 * Add the address field.
 */
static int cdp_add_addr(u_char *buffer, u_int32_t addr) {
	struct cdp_field *field;

	if (!addr)
		return 0;

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_ADDRESS);
	field->length = htons(17);

	/* Number of addresses */
	buffer += sizeof(struct cdp_field);
	*((u_int32_t *)buffer) = htonl(1);
	buffer += sizeof(u_int32_t);
	buffer[0] = 0x01;	   /* Protocol Type = NLPID */
	buffer[1] =	0x01; 	   /* Protocol Length */
	buffer[2] = PROTO_IP;  /* Protocol = IP */
	buffer += 3;
	/* Address Length */
	*((u_int16_t *)buffer) = htons(sizeof(addr));
	/* Address */
	*((u_int32_t *)(buffer+2)) = addr;

	return 17;
}

/**
 * Add the port id field.
 */
static int cdp_add_port_id(u_char *buffer, u_char *port) {
	struct cdp_field *field;

	assert(port);
	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_PORT_ID);
	field->length = htons(strlen(port) + 4);
	buffer += sizeof(struct cdp_field);
	memcpy(buffer, port, strlen(port));

	return strlen(port)+4;
}

/**
 * Add the capabilities field.
 */
static int cdp_add_capabilities(u_char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_CAPABILITIES);
	field->length = htons(sizeof(struct cdp_field) + sizeof(u_int32_t));
	buffer += sizeof(struct cdp_field);
	*((u_int32_t *) buffer) = htonl(cfg.capabilities);
	
	return sizeof(struct cdp_field) + sizeof(u_int32_t);
}

/**
 * Add the software version field.
 */
static int cdp_add_software_version(u_char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_IOS_VERSION);
	field->length = htons(sizeof(struct cdp_field) + strlen(cfg.software_version));
	buffer += sizeof(struct cdp_field);
	memcpy(buffer, cfg.software_version, strlen(cfg.software_version));

	return sizeof(struct cdp_field) + strlen(cfg.software_version);
}

/**
 * Add the platform field. 
 */
static int cdp_add_platform(u_char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *) buffer;
	field->type = htons(TYPE_PLATFORM);
	field->length = htons(sizeof(struct cdp_field) + strlen(cfg.platform));
	buffer += sizeof(struct cdp_field);
	memcpy(buffer, cfg.platform, strlen(cfg.platform));

	return sizeof(struct cdp_field) + strlen(cfg.platform);
}

/**
 * Add the duplex field.
 */
static int cdp_add_duplex(u_char *buffer) {
	struct cdp_field *field;
	
	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_DUPLEX);
	field->length = htons(sizeof(struct cdp_field) + 1);
	buffer += sizeof(struct cdp_field);
	buffer[0] = cfg.duplex;

	return sizeof(struct cdp_field) + 1;
}

static u_int16_t cdp_checksum(u_char *buffer, size_t len) {
	if (len % 2 == 0) {
		return libnet_ip_check((u_int16_t *)buffer, len);
	}
	else {
		int c = buffer[len-1];
		u_int16_t *sp = (u_int16_t *)(&buffer[len-1]);
		u_int16_t r;

		*sp = htons(c);
		r = libnet_ip_check((u_int16_t *)buffer, len+1);
		buffer[len-1] = c;
		return r;
	}
}


static void cdp_send_loop() {
	struct cdp_interface *entry, *tmp;
	int offset, r;

	while (1) {
		dbg("cdp_send_loop()\n");
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			dbg("%s, hw_addr: %02hx:%02hx:%02hx:%02hx:%02hx:%02hx, %p, %p, %p\n",
					entry->name, entry->hwaddr->ether_addr_octet[0],
					entry->hwaddr->ether_addr_octet[1], entry->hwaddr->ether_addr_octet[2],
					entry->hwaddr->ether_addr_octet[3], entry->hwaddr->ether_addr_octet[4],
					entry->hwaddr->ether_addr_octet[5],
					entry, entry->hwaddr, entry->hwaddr->ether_addr_octet);
			offset = cdp_frame_init(data, sizeof(data), entry->hwaddr); 
			offset += cdp_add_device_id(data+offset);
			offset += cdp_add_addr(data+offset, entry->addr);
			offset += cdp_add_port_id(data+offset, entry->name);
			offset += cdp_add_capabilities(data+offset);
			offset += cdp_add_software_version(data+offset);
			offset += cdp_add_platform(data+offset);
			offset += cdp_add_duplex(data+offset);
			/* frame length */
			((struct cdp_frame_header *)data)->length = htons(offset-14);
			/* checksum */
			((struct cdp_hdr *)(data + sizeof(struct cdp_frame_header)))->checksum =
				cdp_checksum(data+sizeof(struct cdp_frame_header),
						offset-sizeof(struct cdp_frame_header));
			if ((r=libnet_write_link(entry->llink, data, offset))!=offset)
				dbg("Wrote only %d bytes (error was: %s).\n", r, strerror(errno));
			dbg("Sent CDP packet of %d bytes on %s.\n", r, entry->name);
		}
		sleep(cfg.timer);
	}
}

int main(int argc, char *argv[]) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *neighbor;
	int fd, maxfd, status;
	fd_set rdfs;
	struct pcap_pkthdr header;
	u_char *packet;
	pid_t pid;

	do_initial_cfg();
	do_initial_register();
	/* hey, no interface in the switch?? */
	assert(!list_empty(&registered_interfaces));


	if (!(pid = fork()))
		cdp_send_loop();
	else {
		dbg("child pid=%d\n", pid);
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

		do {
			dbg("waiting for child, pid=%d to exit.\n", pid);
			waitpid(pid, &status, 0);
		} while (!WIFEXITED(status));
	}

	return 0;
}

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
void decode_alpha_field(u_char *field) {
	dbg("\t\tfield value: '%s'\n", field);
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
void decode_address_field(u_char *field, u_int32_t len) {
	u_int32_t i, number, addr, consumed;

	number = ntohl(*((u_int32_t *)field));
	field += sizeof(number);
	dbg("\t\tnumber of addresses: %d\n", number);
	for (i = 0; i<number; i++) {
		u_char protocol_type = field[0];
		u_char length = field[1]; 
		dbg("\t\t\t%d ptype: %d, length: %d\n", i, protocol_type, length);
		return;
	}
}

void decode_field(int type, int len, u_char *field) {
	dbg("\t[%s], length: %d\n", get_description(type, field_types), len);
	switch (type) {
	case TYPE_DEVICE_ID:
	case TYPE_PORT_ID:
	case TYPE_IOS_VERSION:
	case TYPE_PLATFORM:
	case TYPE_VTP_MGMT_DOMAIN:
		decode_alpha_field(field);
		break;
	case TYPE_ADDRESS:
		decode_address_field(field, len);
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
		decode_field(field_type, field_len, ((u_char *)cdp_field)+sizeof(struct cdp_frame_data));
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

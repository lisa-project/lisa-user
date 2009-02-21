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

#include "cdpd.h"
#include "util.h"
#include "debug.h"

/**
 *  Functions for building and sending cdp frames
 */
 
/* data buffer for cdp frame building */
static unsigned char data[MAX_CDP_FRAME_SIZE];
extern struct cdp_configuration ccfg;
extern struct list_head registered_interfaces;
extern struct cdp_traffic_stats cdp_stats;

/**
 * Fill in the cdp frame header fields.
 */
static int cdp_frame_init(unsigned char *buffer, int len, char *if_name) {
	int sockfd, status;
	struct ifreq ifr;
	struct cdp_frame_header *fhdr;
	struct cdp_hdr *phdr;

	/* open socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sockfd >= 0);

	/* get hardware address */
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
	status = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
	assert(status >= 0);
	
	memset(buffer, 0, len);
	fhdr = (struct cdp_frame_header *)buffer;
	/* dst mac is multicast (01:00:0c:cc:cc:cc) */
	fhdr->dst_addr[0] = 0x01;
	fhdr->dst_addr[1] = 0x00;
	fhdr->dst_addr[2] = 0x0c;
	fhdr->dst_addr[3] = fhdr->dst_addr[4] = fhdr->dst_addr[5] = 0xcc;
	/* fill in src mac address (our mac addres) */
	memcpy(fhdr->src_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	close(sockfd);


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
	phdr->version = ccfg.version;
	/* CDP holdtime */
	phdr->time_to_live = ccfg.holdtime;
	/* Checksum will be calculated later */
	phdr->checksum = 0x00;

	return sizeof(struct cdp_frame_header) + sizeof(struct cdp_hdr);
}

/**
 * Add the device id field.
 */
static int cdp_add_device_id(unsigned char *buffer) {
	char hostname[MAX_HOSTNAME];
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
static int cdp_add_addr(unsigned char *buffer) {
	unsigned int addr = 0;
	struct cdp_field *field;

	/* FIXME: addr should be the mgmt ip */
	/* We should have a ioctl that reads the 
	 management address (addresses) from the 
	 switch configuration */

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_ADDRESS);
	field->length = htons(17);

	/* Number of addresses */
	buffer += sizeof(struct cdp_field);
	*((unsigned int *)buffer) = htonl(1);
	buffer += sizeof(unsigned int);
	buffer[0] = 0x01;	   /* Protocol Type = NLPID */
	buffer[1] =	0x01; 	   /* Protocol Length */
	buffer[2] = PROTO_IP;  /* Protocol = IP */
	buffer += 3;
	/* Address Length */
	*((unsigned short *)buffer) = htons(sizeof(addr));
	/* Address */
	*((unsigned int *)(buffer+2)) = addr;

	return 17;
}

/**
 * Add the port id field.
 */
static int cdp_add_port_id(unsigned char *buffer, char *port) {
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
static int cdp_add_capabilities(unsigned char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_CAPABILITIES);
	field->length = htons(sizeof(struct cdp_field) + sizeof(unsigned int));
	buffer += sizeof(struct cdp_field);
	*((unsigned int *) buffer) = htonl(ccfg.capabilities);
	
	return sizeof(struct cdp_field) + sizeof(unsigned int);
}

/**
 * Add the software version field.
 */
static int cdp_add_software_version(unsigned char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_IOS_VERSION);
	field->length = htons(sizeof(struct cdp_field) + strlen(ccfg.software_version));
	buffer += sizeof(struct cdp_field);
	memcpy(buffer, ccfg.software_version, strlen(ccfg.software_version));

	return sizeof(struct cdp_field) + strlen(ccfg.software_version);
}

/**
 * Add the platform field. 
 */
static int cdp_add_platform(unsigned char *buffer) {
	struct cdp_field *field;

	field = (struct cdp_field *) buffer;
	field->type = htons(TYPE_PLATFORM);
	field->length = htons(sizeof(struct cdp_field) + strlen(ccfg.platform));
	buffer += sizeof(struct cdp_field);
	memcpy(buffer, ccfg.platform, strlen(ccfg.platform));

	return sizeof(struct cdp_field) + strlen(ccfg.platform);
}

/**
 * Add the duplex field.
 */
static int cdp_add_duplex(unsigned char *buffer) {
	struct cdp_field *field;
	
	field = (struct cdp_field *)buffer;
	field->type = htons(TYPE_DUPLEX);
	field->length = htons(sizeof(struct cdp_field) + 1);
	buffer += sizeof(struct cdp_field);
	buffer[0] = ccfg.duplex;

	return sizeof(struct cdp_field) + 1;
}

static unsigned short cdp_checksum(unsigned short *buffer, size_t len) {
	unsigned long sum = 0;

	while (len > 1) {
		sum += *buffer++;
		len -= 2;
	}
	if (len == 1)
		sum += htons(*buffer);

	sum = (sum >> 16) + (sum & 0xffff);

	return (~(sum + (sum >> 16)) & 0xffff);
}

void *cdp_send_loop(void *arg) {
	struct cdp_interface *entry, *tmp;
	int offset, r;

	while (1) {
		sleep(ccfg.timer);
		sys_dbg("cdp_send_loop()\n");
		if (!ccfg.enabled) {
			sys_dbg("cdp is disabled\n");
			continue;
		}
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			offset = cdp_frame_init(data, sizeof(data), entry->name); 
			offset += cdp_add_device_id(data+offset);
			offset += cdp_add_addr(data+offset);
			offset += cdp_add_port_id(data+offset, entry->name);
			offset += cdp_add_capabilities(data+offset);
			offset += cdp_add_software_version(data+offset);
			offset += cdp_add_platform(data+offset);
			offset += cdp_add_duplex(data+offset);
			/* frame length */
			((struct cdp_frame_header *)data)->length = htons(offset-14);
			/* checksum */
			((struct cdp_hdr *)(data + sizeof(struct cdp_frame_header)))->checksum =
				cdp_checksum((unsigned short *)(data+sizeof(struct cdp_frame_header)),
						offset-sizeof(struct cdp_frame_header));
			if ((r=send(entry->sw_sock_fd, data, offset, 0))!=offset)
				sys_dbg("Wrote only %d bytes (error was: %s).\n", r, strerror(errno));
			sys_dbg("Sent CDP packet of %d bytes on %s.\n", r, entry->name);
			/* update cdp out stats */
			if (ccfg.version == 1)
				cdp_stats.v1_out++;
			else
				cdp_stats.v2_out++;
		}
	}
	pthread_exit(NULL);
}

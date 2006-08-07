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
#include "debug.h"

/**
 *  Functions for building and sending cdp frames
 */
 
/* data buffer for cdp frame building */
static unsigned char data[65535];
extern struct cdp_configuration cfg;
extern struct list_head registered_interfaces;
extern struct cdp_traffic_stats cdp_stats;

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
static int cdp_add_port_id(u_char *buffer, char *port) {
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

void *cdp_send_loop(void *arg) {
	struct cdp_interface *entry, *tmp;
	int offset, r;

	while (1) {
		sleep(cfg.timer);
		dbg("cdp_send_loop()\n");
		if (!cfg.enabled) {
			dbg("cdp is disabled\n");
			continue;
		}
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
			/* update cdp out stats */
			if (cfg.version == 1)
				cdp_stats.v1_out++;
			else
				cdp_stats.v2_out++;
		}
	}
	pthread_exit(NULL);
}

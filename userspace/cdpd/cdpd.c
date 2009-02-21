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
#include "cdp.h"
#include "debug.h"
#include "swsock.h"
#include "if_generic.h"
#include "util.h"
#include <signal.h>

/* Two linked lists with the cdp registered and de-registered interfaces */
LIST_HEAD(registered_interfaces);
LIST_HEAD(deregistered_interfaces);

/* cdp configuration parameters (version, ttl, timer) */
struct cdp_configuration ccfg;
/* sender, listener and cleaner threads */
pthread_t sender_thread, listener_thread, cleaner_thread, shutdown_thread;
/* cdp traffic statistics */
struct cdp_traffic_stats cdp_stats;
/* data buffer to read cdp frames into */
static unsigned char packet[MAX_CDP_FRAME_SIZE];

extern neighbor_heap_t *nheap; 					/* cdp neighbor heap (cdp neighbor aging mechanism) */
extern int hend, heap_size;						/* heap end, heap allocated size */ 
extern sem_t nheap_sem;							/* neighbor heap semaphore */
extern char cdp_queue_name[32];					/* the IPC queue id */

extern void *cdp_send_loop(void *);				/* Entry point for the sender thread (cdp_send.c) */
extern void *cdp_ipc_listen(void *);			/* Entry point for the ipc listener thread (cdp_configuration.c) */
/* functions exported from cdp_aging.c */
extern void sift_up(neighbor_heap_t *, int);
extern void sift_down(neighbor_heap_t *, int, int);
extern void *cdp_clean_loop(void *);			/* Entry point for the cdp neighbor cleaner thread */


/* get the description string from a description table */
#ifdef DEBUG
static const char *get_description(unsigned short val, const description_table *table) {
	unsigned short i = 0;

	while (table[i].description) {
		if (table[i].value == val)
			break;
		i++;
	}
	return table[i].description;
}
#endif

/**
 * setup a switch socket.
 */
static int setup_switch_socket(int fd, char *ifname) {
	struct sockaddr_sw addr;

	memset(&addr, 0, sizeof(addr));
	addr.ssw_family = AF_SWITCH;
	strncpy(addr.ssw_if_name, ifname, sizeof(addr.ssw_if_name)-1);
	addr.ssw_proto = ETH_P_CDP;
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("bind");
		close(fd);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	return 0;
}

/**
 * add an interface to the list of cdp-enabled
 * interfaces.
 */
void register_cdp_interface(char *ifname) {
	int sock, status;
	struct ifreq ifr;
	struct swcfgreq swcfgr;
	struct cdp_interface *entry, *tmp;

	sys_dbg("Enabling cdp on '%s'\n", ifname);

	/* Open a socket to request interface status information (up/down) */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock>=0);

	/* get interface flags */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	status = ioctl(sock, SIOCGIFFLAGS, &ifr);
	assert(status>=0);

	/* interface is down */
	if (!(ifr.ifr_flags & IFF_UP)) {
		sys_dbg("interface %s is down.\n", ifname);
		close(sock);
		return;
	}
	close(sock);


	/* Open a switch socket to check if the interface is in the switch */
	sock = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock>=0);

	/* check if the interface is in the switch */
	swcfgr.cmd = SWCFG_GETIFTYPE;
	swcfgr.ifindex = if_get_index(ifname, sock);
	if (ioctl(sock, SIOCSWCFG, &swcfgr)) {
		sys_dbg("interface %s is not in the switch.\n", ifname);
		perror("ioctl");
		close(sock);
		return;
	}

	if (swcfgr.ext.switchport != SW_IF_SWITCHED) {
		sys_dbg("interface %s is virtual.\n", ifname);
		close(sock);
		return;
	}

	/* check if cdp is already enabled on that interface */
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			sys_dbg("CDP already enabled on interface %s.\n", ifname);
			close(sock);
			return;
		}

	/* search for the interface in the deregistered_interfaces list */
	list_for_each_entry_safe(entry, tmp, &deregistered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			/* if setup_switch_socket fails, the interface will not be
			 registered */
			if (setup_switch_socket(sock, ifname))
				return;
			entry->sw_sock_fd = sock;
			list_del(&entry->lh);
			list_add_tail(&entry->lh, &registered_interfaces);
			return;
		}

	/* Bind the switch socket to the interface before
	 allocating the cdp_interface structure */
	if (setup_switch_socket(sock, ifname))
		return;

	/* if not found, we must allocate the structure and do the 
	   proper intialization */
	entry = (struct cdp_interface *) malloc(sizeof(struct cdp_interface));
	assert(entry);
	strncpy(entry->name, ifname, IFNAMSIZ);
	entry->sw_sock_fd = sock;

	status = sem_init(&entry->n_sem, 0, 1);
	assert(!status);
	INIT_LIST_HEAD(&entry->neighbors);

	/* add the entry to the registered_interfaces list */
	list_add_tail(&entry->lh, &registered_interfaces);
}

/**
 * remove an interface from the list of cdp-enabled
 * interfaces.
 */
void unregister_cdp_interface(char *ifname) {
	struct cdp_interface *entry, *tmp;

	/* move the entry to the deregistrated_interfaces list */
	sys_dbg("Disabling cdp on '%s'\n", ifname);
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			/* close the switch socket before moving the entry to
			 the deregistered interfaces list */
			close(entry->sw_sock_fd);
			list_del(&entry->lh);
			list_add_tail(&entry->lh, &deregistered_interfaces);
		}
}

/**
 * Returns 1 if the interface is cdp-enabled, 0 otherwise.
 */
int get_cdp_status(char *ifname) {
	struct cdp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ))
			return 1;
	return 0;
}

/**
 * Identify a cdp neighbor in the neighbor list by its
 * device_id.
 */
static struct cdp_neighbor *find_by_device_id(struct cdp_interface *entry, char *device_id) {
	struct cdp_neighbor *n, *tmp;

	list_for_each_entry_safe(n, tmp, &(entry->neighbors), lh)
		if (!strncmp(n->device_id, device_id, sizeof(n->device_id)))
			return n;
	return NULL;
}

/* Default CDP configuration */
static void do_initial_cfg(void) {
	struct utsname u_name;

	ccfg.enabled = 1;						/* CDP is enabled by default */
	ccfg.version = CFG_DFL_VERSION;			/* CDPv2*/
	ccfg.holdtime = CFG_DFL_HOLDTIME;		/* 180 seconds */
	ccfg.timer = CFG_DFL_TIMER;				/* 60 seconds */
	ccfg.capabilities = CAP_L2SW;	/* advertise as a layer 2 (non-STP) switch */
	ccfg.duplex = 0x01;
	/* get uname information and set software version and platform */
	uname(&u_name);
	memset(ccfg.software_version, 0, sizeof(ccfg.software_version));
	snprintf(ccfg.software_version, sizeof(ccfg.software_version), 
			"Linux %s, %s.", u_name.release, u_name.version);
	ccfg.software_version[sizeof(ccfg.software_version)-1] = '\0';
	memset(ccfg.platform, 0, sizeof(ccfg.platform));
	snprintf(ccfg.platform, sizeof(ccfg.platform), "%s", u_name.machine);
	ccfg.platform[sizeof(ccfg.platform)-1] = '\0';

	/* initialize statistics */
	cdp_stats.v1_in = cdp_stats.v2_in = cdp_stats.v1_out = cdp_stats.v2_out = 0;
}

static inline void print_ipv4_addr(unsigned int addr) {
	sys_dbg("%d.%d.%d.%d", addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
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
static void decode_address(char *field, unsigned int len, struct cdp_neighbor *n) {
	unsigned int i, number, offset;

	number = ntohl(*((unsigned int *)field));
	field += sizeof(number);

	if (number > sizeof(n->addr))
		number = sizeof(n->addr);

	for (i = 0, offset = 0; i<number; i++) {
		unsigned char protocol_type = field[0];
		unsigned char length = field[1]; 
		if (protocol_type != 1 || length != 1) {
			sys_dbg("\t\t\tUnsupported protocol type.");
			field += length;
			continue;
		}
		unsigned char pvalue = field[2]; 
		if (pvalue != PROTO_IP) {
			sys_dbg("\t\t\tOnly IPv4 supported. got pvalue: 0x%x.\n", pvalue);
			field += length;
			continue;
		}
		n->addr[offset++] = ntohl(*((unsigned int *) (field + 5)));
		field += length;
	}
	n->num_addr = offset;
}

static void print_capabilities(unsigned char cap) {
	unsigned short i = 0;

	while (device_capabilities[i].description) {
		if (cap & device_capabilities[i].value) 
			sys_dbg("\t\t[%s]\n", device_capabilities[i].description);
		i++;
	}
}

static void decode_protocol_hello(char *field, unsigned int len, struct protocol_hello *h) {
	h->oui = ntohl(*((unsigned int *)field)) >> 8;
	h->protocol_id = ntohs(*((unsigned short *) (field + 3)));
	memcpy(h->payload, (field + 5), sizeof(h->payload));
}

static void copy_alpha_field(char *dest, char *src, size_t size) {
	strncpy(dest, src, size);
	dest[size-1] = '\0';
}

static void decode_field(struct cdp_neighbor **ne, int type, int len, char *field) {
	struct cdp_neighbor* neighbor = *ne;
	struct cdp_neighbor* n = NULL;
	switch (type) {
	case TYPE_DEVICE_ID:
		n = find_by_device_id(neighbor->interface, field);
		if (n) {
			/* version may have changed */
			n->cdp_version = neighbor->cdp_version;
			free(neighbor);
			sys_dbg("\t\tneighbor %s already registered.\n", field);
			neighbor = n;
			sem_wait(&nheap_sem);
			/* update neighbor heap node timestamp */
			neighbor->hnode->tstamp = neighbor->ttl + time(NULL);
			/* sift down the updated node to keep the heap ordered (min-heap) */
			sift_down(nheap, (neighbor->hnode - nheap)/sizeof(neighbor_heap_t), hend);
			sem_post(&nheap_sem);
			*ne = neighbor;
		}
		else {
			/* a new neighbor was found */
			sys_dbg("\t\tnew neighbor: %s.\n", field);
			copy_alpha_field(neighbor->device_id, field, sizeof(neighbor->device_id));
			sem_wait(&nheap_sem);
			nheap[++hend].tstamp = neighbor->ttl + time(NULL);	
			sys_dbg("\t\ttimestamp: %ld\n", nheap[hend].tstamp);
			nheap[hend].n = neighbor;
			neighbor->hnode = &nheap[hend];
			/* sift up the added node */
			sift_up(nheap, hend);
			if (hend > heap_size) {
				/* grow the heap for more storage */
				heap_size = heap_size + INITIAL_HEAP_SIZE;
				nheap = (neighbor_heap_t *) realloc(nheap, heap_size);
			}
			sem_post(&nheap_sem);
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
		neighbor->cap = ntohl(*((unsigned int *)field));
		break;
	case TYPE_PROTOCOL_HELLO:
		decode_protocol_hello(field, len, &neighbor->p_hello);
		break;
	case TYPE_DUPLEX:
		neighbor->duplex = field[0];
		break;
	case TYPE_NATIVE_VLAN:
		neighbor->native_vlan = ntohs(*((unsigned short *) field));
		break;
	default:
		sys_dbg("\t\twe don't decode [%s].\n", get_description(type, field_types));
	}
}

/* packet introspection function */
/**
 * Trebuie sa existe un hold time pentru o inregistrare de neighbor.
 * Daca holdtime expira -> zboara inregistrarea.
 */
static void dissect_packet(unsigned char *packet, struct cdp_neighbor **neighbor) {
	struct cdp_frame_header *header = (struct cdp_frame_header *)packet;
	struct cdp_hdr *hdr;
	struct cdp_field *field;
	unsigned int packet_length, consumed, field_type, field_len;

	sys_dbg("Received CDP packet on %s", (*neighbor)->interface->name);

	hdr = (struct cdp_hdr *) (((unsigned char *)packet) + 22);
	sys_dbg("\tCDP version: %d, ttl: %d, checksum: %d\n", hdr->version,
			hdr->time_to_live, hdr->checksum);
	/* update in stats */
	if (hdr->version == 1)
		cdp_stats.v1_in++;
	else
		cdp_stats.v2_in++;
	sys_dbg("header->length: %d\n", ntohs(header->length));
	packet_length = (ntohs(header->length) - 8) - sizeof(struct cdp_hdr);
	sys_dbg("\tpacket length: %d\n", packet_length);
	field = (struct cdp_field *) ((((unsigned char *)packet) + 22) + sizeof(struct cdp_hdr));
	(*neighbor)->cdp_version = hdr->version;
	(*neighbor)->ttl = hdr->time_to_live;

	consumed = 0;
	while (consumed < packet_length) {
		field_type = ntohs(field->type);
		field_len = ntohs(field->length);
		decode_field(neighbor, field_type, field_len-sizeof(struct cdp_field), 
				((char *)field)+sizeof(struct cdp_field));
		consumed += field_len;	
		field = (struct cdp_field *) (((unsigned char *)field) + field_len);
	}
	sys_dbg("\n\n");
}

/**
 * Display the contents of the cdp_neighbor structure.
 */
static void print_cdp_neighbor(struct cdp_neighbor *n) {
	int i;
	sys_dbg("CDP neighbor [%p] on %s\n", n, n->interface->name);
	sys_dbg("--------------------------------------\n");
	sys_dbg("\t[%s]: '%s'\n", get_description(TYPE_DEVICE_ID, field_types), n->device_id);
	sys_dbg("\t[%s] (%d):\n", get_description(TYPE_ADDRESS, field_types), n->num_addr);
	for (i=0; i<n->num_addr; i++) {
		sys_dbg("\t\t\t");
		print_ipv4_addr(n->addr[i]);
		sys_dbg("\n");
	}
	sys_dbg("\t[%s]: '%s'\n", get_description(TYPE_PORT_ID, field_types), n->port_id);
	sys_dbg("\t[%s]:\n", get_description(TYPE_CAPABILITIES, field_types));
	print_capabilities(n->cap);
	sys_dbg("\t[%s]: '%s'\n", get_description(TYPE_IOS_VERSION, field_types), n->software_version);
	sys_dbg("\t[%s]: '%s'\n", get_description(TYPE_PLATFORM, field_types), n->platform);
	sys_dbg("\t[%s]: '%s'\n", get_description(TYPE_VTP_MGMT_DOMAIN, field_types), n->vtp_mgmt_domain);
	sys_dbg("\t[%s]:\n", get_description(TYPE_PROTOCOL_HELLO, field_types));
	sys_dbg("\t\tOUI: 0x%02X%02X%02X\n", n->p_hello.oui >> 16, (n->p_hello.oui >> 8) & 0xFF,
			n->p_hello.oui &0xFF);
	sys_dbg("\t\tProtocol ID: 0x%02X%02X\n", n->p_hello.protocol_id >> 8,
			n->p_hello.protocol_id & 0xFF);
	sys_dbg("\t\tpayload len: %d\n", sizeof(n->p_hello.payload));
	sys_dbg("\t\tvalue: ");
	for (i=0; i<sizeof(n->p_hello.payload); i++) {
		sys_dbg("%02X", n->p_hello.payload[i]);
	}
	sys_dbg("\n");
	sys_dbg("\t[%s]: %d\n", get_description(TYPE_DUPLEX, field_types), n->duplex);
	sys_dbg("\t[%s]: %d\n", get_description(TYPE_NATIVE_VLAN, field_types), n->native_vlan);
	sys_dbg("\n");
}

static void cdp_recv_loop(void) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *neighbor;
	int fd, maxfd, status, len;
	fd_set rdfs;

	/* the loop in which we capture the cdp frames */
	for (;;) {
		sys_dbg("[cdp recv loop]\n");
		FD_ZERO(&rdfs);
		maxfd = -1;
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			FD_SET((fd = entry->sw_sock_fd), &rdfs);
			if (fd > maxfd)
				maxfd = fd;
		}
		if (maxfd < 0) { /* no filedescriptor (no registered interface) */
			sleep(1);
			continue;
		}
		status = select(maxfd+1, &rdfs, 0, 0, 0);
		if (status < 0)
			continue;

		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			if (!FD_ISSET(entry->sw_sock_fd, &rdfs))
				continue;
			sys_dbg("data available on %s\n", entry->name);
			if ((len = recv(entry->sw_sock_fd, packet, sizeof(packet), 0)) < 0) {
				perror("recv");
				continue;
			}
			if (!ccfg.enabled) {
				sys_dbg("[cdp receiver]: cdp is disabled\n");
			}
			neighbor = (struct cdp_neighbor *) malloc(sizeof(struct cdp_neighbor));
			neighbor->interface = entry;
			sem_wait(&entry->n_sem);
			dissect_packet(packet, &neighbor);
			sem_post(&entry->n_sem);
			print_cdp_neighbor(neighbor);
		}
	}
}

void *signal_handler(void *ptr) {
	sigset_t signal_set;
	int sig;

	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
	sigaddset(&signal_set, SIGTERM);
	sys_dbg("[signal handler]: Waiting for SIGINT ...\n");
	sigwait(&signal_set, &sig);
	sys_dbg("[signal handler]: Caught SIGINT ... \n");
	sys_dbg("[signal handler]: Removing the IPC queue ... \n");
	sys_dbg("queue_name: '%s'\n", cdp_queue_name);
	mq_unlink(cdp_queue_name);
	unlink(CDPD_PID_FILE);
	closelog();
	sys_dbg("[signal handler]: Exiting\n");
	exit(0);
}

int main(int argc, char *argv[]) {
	FILE *pidfile;
	int status;
	sigset_t signal_set;

	/* daemonize */
	openlog("cdpd", LOG_PID, LOG_DAEMON);
	daemonize();

	/* mask all signals */
	sigfillset(&signal_set);
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

	/* initial cdp configuration */
	do_initial_cfg();

	/* alloc space for the neighbor heap*/
	heap_size = INITIAL_HEAP_SIZE;
	hend = -1;
	nheap = (neighbor_heap_t *) malloc(INITIAL_HEAP_SIZE * sizeof(neighbor_heap_t));
	/* initialize the neighbor heap semaphore */
	status = sem_init(&nheap_sem, 0, 1);
	assert(!status);

	/* start the threads (sender, ipc listener and cleaner) */
	status = pthread_create(&sender_thread, NULL, cdp_send_loop, (void *)NULL);
	assert(!status);
	status = pthread_create(&listener_thread, NULL, cdp_ipc_listen, (void *)NULL);
	assert(!status);
	status = pthread_create(&cleaner_thread, NULL, cdp_clean_loop, (void *)NULL);
	assert(!status);
	status = pthread_create(&shutdown_thread, NULL, signal_handler, (void *)NULL);

	/* create pid file */
	if (!(pidfile = fopen(CDPD_PID_FILE, "w"))) {
		sys_dbg("Failed to open pidfile. Why, oh why?\n");
		exit(1);
	}
	fprintf(pidfile, "%d", getpid());
	fclose(pidfile);


	/* cdp receiver loop */
	cdp_recv_loop();

	return 0;
}

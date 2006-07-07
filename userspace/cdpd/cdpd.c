#include "cdpd.h"
#include "debug.h"

/* Two linked lists with the cdp registered and de-registered interfaces */
LIST_HEAD(registered_interfaces);
LIST_HEAD(deregistered_interfaces);

/* cdp configuration parameters (version, ttl, timer) */
struct cdp_configuration cfg;
/* sender, listener and cleaner threads */
pthread_t sender_thread, listener_thread, cleaner_thread;
/* cdp traffic statistics */
struct cdp_traffic_stats cdp_stats;

extern neighbor_heap_t *nheap; 					/* cdp neighbor heap (mac aging mechanism) */
extern int hend, heap_size;						/* heap end, heap allocated size */ 
extern sem_t nheap_sem;							/* neighbor heap semaphore */

extern void *cdp_send_loop(void *);				/* Entry point for the sender thread (cdp_send.c) */
extern void *cdp_ipc_listen(void *);			/* Entry point for the ipc listener thread (cdp_configuration.c) */
/* function exported from cdp_aging.c */
extern void sift_up(neighbor_heap_t *, int);
extern void sift_down(neighbor_heap_t *, int, int);
extern void *cdp_clean_loop(void *);			/* Entry point for the cdp neighbor cleaner thread */


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
	int sockfd, swsockfd;
	struct ifreq ifr;
	struct net_switch_ioctl_arg ioctl_arg;
	struct cdp_interface *entry, *tmp;
	struct bpf_program filter;
	struct libnet_ether_addr *hw;
	char err[PCAP_ERRBUF_SIZE];
	char buf[128]; 

	dbg("Enabling cdp on '%s'\n", ifname);

	/* check if the interface is virtual */
	if (!strncmp(ifname, LMS_VIRT_PREFIX, strlen(LMS_VIRT_PREFIX))) {
		dbg("interface %s is virtual.\n", ifname);
		return;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sockfd>=0);

	swsockfd = socket(PF_PACKET, SOCK_RAW, 0);
	assert(swsockfd>=0);

	/* get interface flags */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	assert(ioctl(sockfd, SIOCGIFFLAGS, &ifr)>=0);

	/* interface is down */
	if (!(ifr.ifr_flags & IFF_UP)) {
		dbg("interface %s is down.\n", ifname);
		close(sockfd);
		close(swsockfd);
		return;
	}
	close(sockfd);

	/* check if the interface is in the switch */
	ioctl_arg.cmd = SWCFG_GETIFCFG;
	ioctl_arg.if_name = strdup(ifname);
	ioctl_arg.ext.cfg.forbidden_vlans = NULL;
	ioctl_arg.ext.cfg.description = NULL;
	if (ioctl(swsockfd, SIOCSWCFG, &ioctl_arg)) {
		dbg("interface %s is not in the switch.\n", ifname);
		perror("ioctl");
		close(swsockfd);
		return;
	}
	close(swsockfd);

	/* check if cdp is already enabled on that interface */
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(ifname, entry->name, IFNAMSIZ)) {
			dbg("CDP already enabled on interface %s.\n", ifname);
			return;
		}

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
	entry->hwaddr = (struct libnet_ether_addr *) malloc(sizeof(struct libnet_ether_addr)); 
	hw = libnet_get_hwaddr(entry->llink);
	if (!hw) {
		fprintf(stderr, "Libnet failed getting hw addr of %s: %s\n", entry->name, err);
		exit(1);
	}
	memcpy(entry->hwaddr, hw, sizeof(struct libnet_ether_addr));
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

	assert(!sem_init(&entry->n_sem, 0, 1));
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
	FILE *f;
	char buf[512], name[IFNAMSIZ];

	if (!(f = fopen(PROCNETDEV_PATH, "r"))) {
		perror("fopen");
		exit(1);
	}

	/* skip 2 lines */
	fgets(buf, sizeof(buf), f);
	fgets(buf, sizeof(buf), f);

	while (fgets(buf, sizeof(buf), f)) {
		fetch_if_name(name, buf);
		register_cdp_interface(name);
	}
	fclose(f);
}

/* Default CDP configuration */
static void do_initial_cfg() {
	struct utsname u_name;

	cfg.enabled = 1;				/* CDP is enabled by default */
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

	/* initialize statistics */
	cdp_stats.v1_in = cdp_stats.v2_in = cdp_stats.v1_out = cdp_stats.v2_out = 0;
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
			/* version may have changed */
			n->cdp_version = neighbor->cdp_version;
			free(neighbor);
			dbg("\t\tneighbor %s already registered.\n", field);
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
			dbg("\t\tnew neighbor: %s.\n", field);
			copy_alpha_field(neighbor->device_id, field, sizeof(neighbor->device_id));
			sem_wait(&nheap_sem);
			nheap[++hend].tstamp = neighbor->ttl + time(NULL);	
			dbg("\t\ttimestamp: %ld\n", nheap[hend].tstamp);
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
	dbg("\tCDP version: %d, ttl: %d, checksum: %d\n", hdr->version,
			hdr->time_to_live, hdr->checksum);
	/* update in stats */
	if (hdr->version == 1)
		cdp_stats.v1_in++;
	else
		cdp_stats.v2_in++;
	packet_length = (header->len - 22) - sizeof(struct cdp_hdr);
	dbg("\tpacket length: %d\n", packet_length);
	field = (struct cdp_field *) ((((u_char *)packet) + 22) + sizeof(struct cdp_hdr));
	(*neighbor)->cdp_version = hdr->version;
	(*neighbor)->ttl = hdr->time_to_live;

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

static void cdp_recv_loop() {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *neighbor;
	int fd, maxfd;
	fd_set rdfs;
	struct pcap_pkthdr header;
	u_char *packet;

	/* the loop in which we capture the cdp frames */
	for (;;) {
		dbg("[cdp recv loop]\n");
		FD_ZERO(&rdfs);
		maxfd = -1;
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			FD_SET((fd = pcap_fileno(entry->pcap)), &rdfs);
			if (fd > maxfd)
				maxfd = fd;
		}
		if (maxfd < 0) { /* no filedescriptor (no registered interface) */
			sleep(1);
			continue;
		}
		select(maxfd+1, &rdfs, 0, 0, 0);
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			packet = (u_char *) pcap_next(entry->pcap, &header);
			if (!cfg.enabled) {
				dbg("[cdp receiver]: cdp is disabled\n");
			}
			if (packet && cfg.enabled) {
				neighbor = (struct cdp_neighbor *) malloc(sizeof(struct cdp_neighbor));
				neighbor->interface = entry;
				sem_wait(&entry->n_sem);
				dissect_packet(&header, packet, &neighbor);
				sem_post(&entry->n_sem);
#ifdef DEBUG 
				print_cdp_neighbor(neighbor);
#endif
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int status;

	do_initial_cfg();
	do_initial_register();

	/* we fail if the list of registered interfaces is empty  */
	//assert(!list_empty(&registered_interfaces));

	/* alloc space for the neighbor heap*/
	heap_size = INITIAL_HEAP_SIZE;
	hend = -1;
	nheap = (neighbor_heap_t *) malloc(INITIAL_HEAP_SIZE * sizeof(neighbor_heap_t));
	/* initialize the neighbor heap semaphore */
	assert(!sem_init(&nheap_sem, 0, 1));

	/* start the threads (sender, ipc listener and cleaner) */
	status = pthread_create(&sender_thread, NULL, cdp_send_loop, (void *)NULL);
	assert(!status);
	status = pthread_create(&listener_thread, NULL, cdp_ipc_listen, (void *)NULL);
	assert(!status);
	status = pthread_create(&cleaner_thread, NULL, cdp_clean_loop, (void *)NULL);
	assert(!status);

	/* cdp receiver loop */
	cdp_recv_loop();

	/* FIXME: clean shutdown mechanism */
	return 0;
}

#include "cdpd.h"
#include "cdp_ipc.h"
#include "cdp.h"
#include "climain.h"

/* selected interface in interface configuration mode */
extern char sel_eth[IFNAMSIZ];
/* defined in if.c */
extern int cmd_int_eth_status(FILE *, char *);

static int cdp_is_disabled(FILE *out) {
	if (!cdp_s.enabled) {
		fprintf(out, "%% CDP is not enabled");
		return 1;
	}
	return 0;
}

int get_cdp_configuration(struct cdp_configuration *conf) {
	struct cdp_request  m;

	if (!cdp_s.enabled)
		return 0;

	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_QUERY;
	m.pid  = getpid();
	m.query.show.type = CDP_SHOW_CFG;

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return 1;
	}

	if (cdp_ipc_receive(&cdp_s))
		return 1;

	memcpy(conf, cdp_s.cdp_response, sizeof(struct cdp_configuration));

	return 0;
}

static int get_cdp_neighbors(char *interface, char *device_id) {
	struct cdp_request m;

	if (!cdp_s.enabled)
		return 0;

	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_QUERY;
	m.pid  = getpid();
	m.query.show.type = CDP_SHOW_NEIGHBORS;

	if (interface) {
		strncpy(m.query.show.interface, interface, strlen(interface));
		m.query.show.interface[strlen(interface)] = '\0';
	}
	if (device_id) {
		strncpy(m.query.show.device_id, device_id, strlen(device_id));
		m.query.show.device_id[strlen(device_id)] = '\0';
	}

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return 1;
	}

	if (cdp_ipc_receive(&cdp_s))
		return 1;

	return 0;
}

static int get_cdp_interfaces(char *interface) {
	struct cdp_request m;

	if (!cdp_s.enabled)
		return 0;

	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_QUERY;
	m.pid  = getpid();
	m.query.show.type = CDP_SHOW_INTF;

	if (interface) {
		strncpy(m.query.show.interface, interface, strlen(interface));
		m.query.show.interface[strlen(interface)] = '\0';
	}

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return 1;
	}

	if (cdp_ipc_receive(&cdp_s))
		return 1;

	return 0;
}

static void print_sh_neighbors_header(FILE *out) {
	fprintf(out, "Capability codes: R - Router, T - Trans Bridge, B - Source Route Bridge\n"
			"\t\tS - Switch, H - Host, I - IGMP, r - Repeater\n\n");
	fprintf(out, "Device ID        Local Intrfce     Holdtme    Capability  Platform            Port ID\n");
}

static void print_neighbors_brief(FILE *out, char *ptr) {
	int i, num = *((int *) ptr);


	ptr = ptr + sizeof(int);
	for (i = 0; i < num; i++) {
		struct cdp_ipc_neighbor *ne = (struct cdp_ipc_neighbor *) (ptr + i*sizeof(struct cdp_ipc_neighbor));
		ne->n.device_id[17] = '\0';
		fprintf(out, "%-17s", ne->n.device_id);
		ne->interface[18] = '\0';
		fprintf(out, "%-18s", ne->interface); 
		fprintf(out, " %-11d", ne->n.ttl);
		int j = 0;
		int numcap = 0;
		int cap = ne->n.cap;
		while (device_capabilities_brief[j].description) {
			if (cap & device_capabilities_brief[j].value) {
				fprintf(out, "%2s", device_capabilities_brief[j].description);
				numcap++;
			}
			j++;
		}
		for (j=0; j<11-2*numcap; j++)
			fprintf(out, " ");
		ne->n.platform[20] = '\0';
		fprintf(out, "%-20s", ne->n.platform);
		fprintf(out, "%-s\n", ne->n.port_id);
	}
}

static void print_ipv4_addr(FILE *out, unsigned int addr) {
	fprintf(out, "%d.%d.%d.%d", addr >> 24, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
}

static void print_neighbors_detail(FILE *out, char *ptr) {
	int i, j, num = *((int *) ptr);


	ptr = ptr + sizeof(int);
	for (i = 0; i < num; i++) {
		struct cdp_ipc_neighbor *ne = (struct cdp_ipc_neighbor *) (ptr + i*sizeof(struct cdp_ipc_neighbor));
		fprintf(out, "-------------------------\n");
		fprintf(out, "Device ID: %s\n", ne->n.device_id);
		if (ne->n.num_addr) {
			fprintf(out, "Entry Address(es):\n");
			for (j=0; j < ne->n.num_addr; j++) { 
				fprintf(out, "  IP address: ");
				print_ipv4_addr(out, ne->n.addr[j]);
				fprintf(out, "\n");
			}
		}
		fprintf(out, "Platform: %s,  Capabilities: ", ne->n.platform);
		j = 0;
		int cap = ne->n.cap;
		while (device_capabilities[j].description) {
			if (cap & device_capabilities[j].value) {
				fprintf(out, "%s ", device_capabilities[j].description);
			}
			j++;
		}
		fprintf(out, "\n");
		fprintf(out, "Interface: %s,  Port ID (outgoing port): %s\n",
				ne->interface, ne->n.port_id);
		fprintf(out, "Holdtime : %d sec\n", ne->n.ttl);
		fprintf(out, "\n");
		fprintf(out, "Version :\n");
		fprintf(out, "%s\n\n", ne->n.software_version);
		fprintf(out, "advertisement version: %d\n", ne->n.cdp_version);
		fprintf(out, "Protocol Hello: OUI=0x%02X%02X%02X, Protocol ID=0x%02X%02X; payload len=%d, value=",
				ne->n.p_hello.oui >> 16, (ne->n.p_hello.oui >> 8) & 0xFF, ne->n.p_hello.oui &0xFF,
				ne->n.p_hello.protocol_id >> 8, ne->n.p_hello.protocol_id & 0xFF,
				sizeof(ne->n.p_hello.payload));
		for (j=0; j<sizeof(ne->n.p_hello.payload); j++)
			fprintf(out, "%02X", ne->n.p_hello.payload[j]);
		fprintf(out, "\n");
		fprintf(out, "VTP Management Domain: '%s'\n", ne->n.vtp_mgmt_domain);
		fprintf(out, "Native VLAN: %d\n", ne->n.native_vlan);
		fprintf(out, "Duplex: %s\n", ne->n.duplex? "full" : "half");
		fprintf(out, "\n");
	}
}

static void print_entries(FILE *out, char *ptr, char proto, char version) {
	int i, j, num = *((int *) ptr);


	ptr = ptr + sizeof(int);
	for (i = 0; i < num; i++) {
		struct cdp_ipc_neighbor *ne = (struct cdp_ipc_neighbor *) (ptr + i*sizeof(struct cdp_ipc_neighbor));
		if (proto && ne->n.num_addr) {
			fprintf(out, "Protocol information for %s :\n", ne->n.device_id);
			for (j=0; j < ne->n.num_addr; j++) { 
				fprintf(out, "  IP address: ");
				print_ipv4_addr(out, ne->n.addr[j]);
				fprintf(out, "\n");
			}
		}
		if (version) {
			fprintf(out, "\nVersion information for %s :\n", ne->n.device_id);
			fprintf(out, "%s\n\n", ne->n.software_version);
		}
	}
}

void cmd_sh_cdp(FILE *out, char **argv) {
	struct cdp_configuration conf; 

	if (cdp_is_disabled(out))
		return;

	if (get_cdp_configuration(&conf))
		return;
	fprintf(out, "Global CDP information:\n"
		"\tSending CDP packets every %d seconds\n"
		"\tSending a holdtime value of %d seconds\n"
		"\tSending CDPv2 advertisements is %s\n",
		conf.timer, conf.holdtime, conf.version==2? "enabled" : "disabled");
}

void cmd_sh_cdp_int(FILE *out, char **argv) {
	struct cdp_configuration conf;
	char *interface = NULL, *ptr;
	int i, count;

	if (cdp_is_disabled(out))
		return;

	if (get_cdp_configuration(&conf))
		return;

	if (argv[0])
		interface = if_name_eth(argv[0]);

	if (get_cdp_interfaces(interface))
		return;

	count = *((int *) cdp_s.cdp_response);

	ptr = cdp_s.cdp_response + sizeof(int);
	for (i = 0; i<count; i++) {
		if (!cmd_int_eth_status(out, ptr))
			fprintf(out, "\tEncapsulation ARPA\n"
					"\tSending CDP packets every %d seconds\n"
					"\tHoldtime is %d seconds\n",
					conf.timer, conf.holdtime);
		ptr += strlen(ptr)+1;
	}
}

void cmd_sh_cdp_ne(FILE *out, char **argv) {
	if (cdp_is_disabled(out))
		return;

	if (get_cdp_neighbors(NULL, NULL))
		return;

	print_sh_neighbors_header(out);
	print_neighbors_brief(out, cdp_s.cdp_response);
}

void cmd_sh_cdp_ne_int(FILE *out, char **argv) {
	if (cdp_is_disabled(out))
		return;

	fprintf(out, "show cdp neighbors ethernet %s\n", argv[0]);
	if (get_cdp_neighbors(if_name_eth(argv[0]), NULL)) 
		return;

	print_sh_neighbors_header(out);
	print_neighbors_brief(out, cdp_s.cdp_response);
}

void cmd_sh_cdp_ne_detail(FILE *out, char **argv) {
	char *interface;

	if (cdp_is_disabled(out))
		return;

	interface = argv[0]? if_name_eth(argv[0]) : NULL;

	if (get_cdp_neighbors(interface, NULL))
		return;

	print_neighbors_detail(out, cdp_s.cdp_response);
}

void cmd_sh_cdp_entry(FILE *out, char **argv) {
	char proto = 0, version = 0;
	char *entry = NULL;
	int i = 0;

	if (cdp_is_disabled(out))
		return;

	while (argv[i]) {
		if (!strncmp(argv[i], "protocol", strlen(argv[i])))
			proto++;
		else if (!strncmp(argv[i], "version", strlen(argv[i])))
			version++;
		else if (strcmp(argv[i], "*"))
			entry = argv[i];
		i++;
	}
	if (get_cdp_neighbors(NULL, entry))
		return;
	if (!proto && !version)
		print_neighbors_detail(out, cdp_s.cdp_response);
	else
		print_entries(out, cdp_s.cdp_response, proto, version);
}

void cmd_sh_cdp_holdtime(FILE *out, char **argv) {
	struct cdp_configuration conf; 

	if (cdp_is_disabled(out))
		return;

	if (get_cdp_configuration(&conf))
		return;
	fprintf(out, "%d secs\n", conf.holdtime);
}

void cmd_sh_cdp_run(FILE *out, char **argv) {
	fprintf(out, "CDP is %s\n", cdp_s.enabled? "enabled" : "disabled");
}

void cmd_sh_cdp_timer(FILE *out, char **argv) {
	struct cdp_configuration conf; 

	if (cdp_is_disabled(out))
		return;
	if (get_cdp_configuration(&conf))
		return;
	fprintf(out, "%d secs\n", conf.timer);
}

void cmd_sh_cdp_traffic(FILE *out, char **argv) {
	struct cdp_request m;
	struct cdp_traffic_stats *stats;

	if (cdp_is_disabled(out))
		return;

	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_QUERY;
	m.pid  = getpid();
	m.query.show.type = CDP_SHOW_STATS;

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return;
	}

	if (cdp_ipc_receive(&cdp_s))
		return;

	stats = (struct cdp_traffic_stats *) cdp_s.cdp_response;
	fprintf(out, "CDP counters:\n"
			"\tTotal packets output: %u, Input: %u\n"
			"\tCDP version 1 advertisements output: %u, Input: %u\n"
			"\tCDP version 2 advertisements output: %u, Input: %u\n",
			stats->v1_out + stats->v2_out, stats->v1_in + stats->v2_in,
			stats->v1_out, stats->v1_in, stats->v2_out, stats->v2_in);
}

static int do_configuration_query(int field_id, int value) {
	struct cdp_request  m;

	if (!cdp_s.enabled)
		return 0;

	memset(&m, 0, sizeof(m));
	m.type = CDP_CONF_QUERY;
	m.pid  = getpid();
	m.query.conf.field_id = field_id;
	m.query.conf.field_value = value;

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return 1;
	}

	if (cdp_ipc_receive(&cdp_s))
		return 1;

	return 0;
}

int cdp_adm_query(int query_type, char *interface) {
	struct cdp_request  m;

	if (!cdp_s.enabled)
		return 0;

	memset(&m, 0, sizeof(m));
	m.type = CDP_ADM_QUERY;
	m.pid  = getpid();
	m.query.adm.type = query_type;
	strncpy(m.query.adm.interface, interface, IFNAMSIZ);
	m.query.adm.interface[IFNAMSIZ-1] = '\0';

	if (mq_send(cdp_s.sq, (const char *)&m, sizeof(m), 0) < 0) {
		perror("mq_send");
		return 1;
	}

	if (cdp_ipc_receive(&cdp_s))
		return 1;

	return 0;
}

void cmd_cdp_version(FILE *out, char **argv) {
	do_configuration_query(CDP_CFG_VERSION, 2);
}

void cmd_cdp_run(FILE *out, char **argv) {
	/* disable / re-enable cdp ipc */
	cdp_destroy_ipc(&cdp_s);
	if (cdp_init_ipc(&cdp_s))
		return;
	if (!do_configuration_query(CDP_CFG_ENABLED, 1))
		cdp_s.enabled = 1;
}

void cmd_cdp_holdtime(FILE *out, char **argv) {
	do_configuration_query(CDP_CFG_HOLDTIME, atoi(argv[0]));
}

void cmd_cdp_timer(FILE *out, char **argv) {
	do_configuration_query(CDP_CFG_TIMER, atoi(argv[0]));
}

void cmd_no_cdp_v2(FILE *out, char **argv) {
	do_configuration_query(CDP_CFG_VERSION, 1);
}

void cmd_no_cdp_run(FILE *out, char **argv) {
	if (!do_configuration_query(CDP_CFG_ENABLED, 0)) {
		cdp_destroy_ipc(&cdp_s);
		cdp_s.enabled = 0;
	}
}

void cmd_cdp_if_enable(FILE *out, char **argv) {
	cdp_adm_query(CDP_IF_ENABLE, sel_eth);
}

void cmd_cdp_if_disable(FILE *out, char **argv) {
	cdp_adm_query(CDP_IF_DISABLE, sel_eth);
}

int cdp_if_is_enabled(char *ifname) {
	if (!cdp_s.enabled)
		return cdp_s.enabled;
	cdp_adm_query(CDP_IF_STATUS, ifname);
	return *((int*) cdp_s.cdp_response);
}

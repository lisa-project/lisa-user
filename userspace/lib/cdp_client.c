#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"
#include "if_generic.h"
#include "cdp_client.h"

struct cdp_session *cdp_session_start(void)
{
	struct cdp_session *s;
	struct mq_attr attr;

	/* Be paranoid about user input */
	s = (struct cdp_session *)malloc(sizeof(struct cdp_session));
	assert(s);
	memset(s, 0, sizeof(struct cdp_session));

	/* Try to open the send message queue on which we send requests to cdpd */
	memset(s->sq_name, 0, sizeof(s->sq_name));
	snprintf(s->sq_name, sizeof(s->sq_name), CDP_QUEUE_NAME, 0);
	s->sq_name[sizeof(s->sq_name)-1] = '\0';

	if ((s->sq = mq_open(s->sq_name, O_WRONLY)) < 0)
		goto out_err;

	/* Open the message queue for receiving responses from cdpd */
	memset(s->rq_name, 0, sizeof(s->rq_name));
	snprintf(s->rq_name, sizeof(s->rq_name), CDP_QUEUE_NAME, getpid());
	s->rq_name[sizeof(s->rq_name)-1] = '\0';

	if ((s->rq = mq_open(s->rq_name, O_CREAT|O_RDONLY, 0666, NULL)) < 0)
		goto out_err;

	/* Get the max message size attribute */
	if (mq_getattr(s->rq, &attr) < 0)
		goto out_err;

	s->max_msg_len = attr.mq_msgsize;

	/* Alloc the receive message buffer */
	if (!(s->response = malloc(s->max_msg_len)))
		goto out_err;

	return s;

out_err:
	cdp_session_end(s);
	return NULL;
}

void cdp_session_end(struct cdp_session *s)
{
	assert(s);

	if (s->response)
		free(s->response);
	mq_close(s->rq);
	mq_unlink(s->rq_name);

	return;
}

int cdp_session_recv(struct cdp_session *s)
{
	struct timespec ts;
	time_t ns;
	int err;

	assert(s);

	memset(&ts, 0, sizeof(ts));
	time(&ns);
	ts.tv_sec = ns + CDP_CLIENT_TIMEOUT;

	if ((err = mq_timedreceive(s->rq, s->response,  s->max_msg_len, NULL, &ts)) < 0) {
		fprintf(stderr, "No message received from cdpd in %d seconds.\n"
			"Disabling cdp in this session.\n", CDP_CLIENT_TIMEOUT);
	}

	return err;
}

int cdp_get_neighbors(struct cdp_session *session, int if_index, char *device_id)
{
	struct cdp_request m;
	int err;

	assert(session);
	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_NEIGHBORS;
	m.pid  = getpid();
	m.if_index = if_index;

	if (device_id) {
		strncpy(m.device_id, device_id, strlen(device_id));
		m.device_id[strlen(device_id)] = '\0';
	}

	if ((err = cdp_session_send(session, m)) < 0) {
		perror("cdp_session_send");
		return err;
	}

	return cdp_session_recv(session);
}

int cdp_get_interfaces(struct cdp_session *session, int if_index)
{
	struct cdp_request m;
	int err;

	assert(session);
	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_INTF;
	m.pid  = getpid();
	m.if_index = if_index;

	if ((err = cdp_session_send(session, m)) < 0) {
		perror("cdp_session_send");
		return err;
	}

	return cdp_session_recv(session);
}

int cdp_get_stats(struct cdp_session *session, struct cdp_traffic_stats *stats)
{
	struct cdp_request m;
	int err;

	assert(session);
	assert(stats);
	memset(&m, 0, sizeof(m));
	m.type = CDP_SHOW_STATS;
	m.pid  = getpid();

	if ((err = cdp_session_send(session, m)) < 0) {
		perror("cdp_session_send");
		return err;
	}

	if ((err = cdp_session_recv(session)) < 0)
		return err;

	stats = (struct cdp_traffic_stats *)session->response;

	return err;
}

void cdp_print_neighbors_detail(struct cdp_session *session, FILE *out)
{
	char *ptr, buf[IFNAMSIZ];
	int i, j, num, sock_fd;
	struct {
		unsigned char val;
		const char *desc;
	} capabilities[] = DEVICE_CAPABILITIES;

	assert(session);
	ptr = session->response;
	num = *((int *) ptr);
	ptr = ptr + sizeof(int);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock_fd != -1);

	for (i = 0; i < num; i++) {
		struct cdp_neighbor_info *n = (struct cdp_neighbor_info *) (ptr + i*sizeof(struct cdp_neighbor_info));
		fprintf(out, "-------------------------\n");
		fprintf(out, "Device ID: %s\n", n->device_id);
		if (n->num_addr) {
			fprintf(out, "Entry Address(es):\n");
			for (j=0; j < n->num_addr; j++) {
				fprintf(out, "  IP address: ");
				fprintf(out, "%u.%u.%u.%u", NIP_QUAD(n->addr[j]));
				fprintf(out, "\n");
			}
		}
		fprintf(out, "Platform: %s,  Capabilities: ", n->platform);
		for (j=0; capabilities[j].desc; j++)
			if (n->cap & capabilities[j].val)
				fprintf(out, "%s ", capabilities[j].desc);
		fprintf(out, "\n");
		fprintf(out, "Interface: %s,  Port ID (outgoing port): %s\n",
				if_get_name(n->if_index, sock_fd, buf), n->port_id);
		fprintf(out, "Holdtime : %d sec\n", n->ttl);
		fprintf(out, "\n");
		fprintf(out, "Version :\n");
		fprintf(out, "%s\n\n", n->software_version);
		fprintf(out, "advertisement version: %d\n", n->cdp_version);
		fprintf(out, "Protocol Hello: OUI=0x%02X%02X%02X, Protocol ID=0x%02X%02X; payload len=%d, value=",
				n->oui >> 16, (n->oui >> 8) & 0xFF, n->oui &0xFF,
				n->protocol_id >> 8, n->protocol_id & 0xFF,
				sizeof(n->payload));
		for (j=0; j<sizeof(n->payload); j++)
			fprintf(out, "%02X", n->payload[j]);
		fprintf(out, "\n");
		fprintf(out, "VTP Management Domain: '%s'\n", n->vtp_mgmt_domain);
		fprintf(out, "Native VLAN: %d\n", n->native_vlan);
		fprintf(out, "Duplex: %s\n", n->duplex? "full" : "half");
		fprintf(out, "\n");
	}
	close(sock_fd);
}

void cdp_print_neighbors_filtered(struct cdp_session *session, FILE *out, char proto, char version)
{
	int i, j, num;
	char *ptr;

	assert(session);
	ptr = session->response;
	num = *((int *) ptr);
	ptr = ptr + sizeof(int);

	for (i = 0; i < num; i++) {
		struct cdp_neighbor_info *n = (struct cdp_neighbor_info *) (ptr + i*sizeof(struct cdp_neighbor_info));
		if (proto && n->num_addr) {
			fprintf(out, "Protocol information for %s :\n", n->device_id);
			for (j=0; j < n->num_addr; j++) {
				fprintf(out, "  IP address: ");
				fprintf(out, "%u.%u.%u.%u", NIP_QUAD(n->addr[j]));
				fprintf(out, "\n");
			}
		}
		if (version) {
			fprintf(out, "\nVersion information for %s :\n", n->device_id);
			fprintf(out, "%s\n\n", n->software_version);
		}
	}
}

void cdp_print_neighbors_brief(struct cdp_session *session, FILE *out) {
	int i, num, sock_fd;
	char *ptr, buf[IFNAMSIZ];
	struct {
		unsigned char val;
		const char *desc;
	} capabilities[] = DEVICE_CAPABILITIES_BRIEF;

	assert(session);
	ptr = session->response;
	num = *((int *) ptr);
	ptr = ptr + sizeof(int);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock_fd != -1);

	fprintf(out, "Capability codes: R - Router, T - Trans Bridge, B - Source Route Bridge\n"
			"\t\tS - Switch, H - Host, I - IGMP, r - Repeater\n\n");
	fprintf(out, "Device ID        Local Intrfce     Holdtme    Capability  Platform            Port ID\n");

	for (i = 0; i < num; i++) {
		struct cdp_neighbor_info *n = (struct cdp_neighbor_info *) (ptr + i*sizeof(struct cdp_neighbor_info));
		int j, numcap = 0;
		n->device_id[17] = '\0';
		fprintf(out, "%-17s", n->device_id);
		fprintf(out, "%-18s", if_get_name(n->if_index, sock_fd, buf));
		fprintf(out, " %-11d", n->ttl);
		for (j=0; capabilities[j].desc; j++)
			if (n->cap & capabilities[j].val) {
				fprintf(out, "%2s", capabilities[j].desc);
				numcap++;
			}
		for (j=0; j<11-2*numcap; j++)
			fprintf(out, " ");
		n->platform[20] = '\0';
		fprintf(out, "%-20s", n->platform);
		fprintf(out, "%-s\n", n->port_id);
	}
	close(sock_fd);
}

#include <linux/net_switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include "cdp_ipc.h"

/* Forks, closes all file descriptors and redirects stdin/stdout to
 * /dev/null */
void daemonize(void) {
	struct rlimit rl = {0};
	int fd = -1;
	int i;

	switch (fork()) {
	case -1:
		syslog(LOG_ERR, "Prefork stage 1: %m");
		exit(1);
	case 0: /* child */
		break;
	default: /* parent */
		exit(0);
	}

	rl.rlim_max = 0;
	getrlimit(RLIMIT_NOFILE, &rl);
	switch (rl.rlim_max) {
	case -1: /* oops! */
		syslog(LOG_ERR, "getrlimit");
		exit(1);
	case 0:
		syslog(LOG_ERR, "Max number of open file descriptors is 0!");
		exit(1);
	}
	for (i = 0; i < rl.rlim_max; i++)
		close(i);
	if (setsid() == -1) {
		syslog(LOG_ERR, "setsid failed");
		exit(1);
	}
	switch (fork()) {
	case -1:
		syslog(LOG_ERR, "Prefork stage 2: %m");
		exit(1);
	case 0: /* child */
		break;
	default: /* parent */
		exit(0);
	}

	chdir("/");
	umask(0);
	fd = open("/dev/null", O_RDWR);
	dup(fd);
	dup(fd);
}

/* Initiates a cdp client session */
int cdp_init_ipc(struct cdp_session_info *s) {
	struct mq_attr attr;
	int    status = 0;

	/* Be paranoid about user input */
	assert(s);
	memset(s, 0, sizeof(struct cdp_session_info));

	/* Try to open the send message queue on which we send requests to cdpd */
	memset(s->sq_name, 0, sizeof(s->sq_name));
	snprintf(s->sq_name, sizeof(s->sq_name), CDP_QUEUE_NAME, 0);
	s->sq_name[sizeof(s->sq_name)-1] = '\0';

	if ((s->sq = mq_open(s->sq_name, O_WRONLY)) < 0)
		return s->sq;

	/* Open the message queue for receiving responses from cdpd */
	memset(s->rq_name, 0, sizeof(s->rq_name));
	snprintf(s->rq_name, sizeof(s->rq_name), CDP_QUEUE_NAME, getpid());
	s->rq_name[sizeof(s->rq_name)-1] = '\0';

	if ((s->rq = mq_open(s->rq_name, O_CREAT|O_RDONLY, 0666, NULL)) < 0)
		return s->rq;

	/* Get the max message size attribute */
	if ((status = mq_getattr(s->rq, &attr)) < 0)
		return status;

	s->max_msg_len = attr.mq_msgsize;

	/* Alloc the receive message buffer */
	if (!(s->cdp_response = malloc(s->max_msg_len)))
		return -ENOMEM;

	s->enabled = 1;

	return status;
}

/* Ends a cdp client session */
void cdp_destroy_ipc(struct cdp_session_info *s) {
	assert(s);

	if (!s->enabled)
		return;

	if (s->cdp_response)
		free(s->cdp_response);
	mq_close(s->rq);
	mq_unlink(s->rq_name);

	return;
}

/* timed receive for a message from the client queue */
int cdp_ipc_receive(struct cdp_session_info *s) {
	struct timespec ts;
	time_t ns;

	assert(s);

	memset(&ts, 0, sizeof(ts));
	time(&ns);
	ts.tv_sec = ns + CDP_CLIENT_TIMEOUT_S;

	if (mq_timedreceive(s->rq, s->cdp_response,  s->max_msg_len, NULL, &ts) < 0) {
		fprintf(stderr, "No message received from cdpd in %d seconds.\n"
			"Disabling cdp in this session.\n", CDP_CLIENT_TIMEOUT_S);
		s->enabled = 0;
		return 1;
	}

	return 0;
}

void cmd_showmac(FILE *out, char *arg)  {
	struct net_switch_mac *mac;
	struct swcfgreq *user_arg = (struct swcfgreq *)arg;
	int size = 0, actual_size = 0; //user_arg->ext.marg.actual_size;
	char *buf = user_arg->ext.marg.buf;

	
	fprintf(out, "Destination Address  Address Type  VLAN  Destination Port\n"
			"-------------------  ------------  ----  ----------------\n");
	while (size < actual_size) {
		mac = (struct net_switch_mac *)(buf + size);
		fprintf(out, "%02x%02x.%02x%02x.%02x%02x       "
				"%12s  %4d  %s\n", 
				mac->addr[0], mac->addr[1], mac->addr[2],
				mac->addr[3], mac->addr[4], mac->addr[5],
			    (mac->addr_type)? "Static" : "Dynamic",
				mac->vlan,
				mac->port
				);
		size += sizeof(struct net_switch_mac);
	}
}

#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <syslog.h>

#include "util.h"

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

void print_mac(FILE *out, void *buf, int size, char *(*get_if_name)(int, void*), void *priv)
{
	struct net_switch_mac *mac, *end =
		(struct net_switch_mac *)((char *)buf + size);

	fprintf(out,
			"Destination Address  Address Type  VLAN  Destination Port\n"
			"-------------------  ------------  ----  ----------------\n");
	for (mac = buf; mac < end; mac++) {
		char *name = NULL;
		if (get_if_name)
			name = get_if_name(mac->ifindex, priv);
		fprintf(out, "%02x%02x.%02x%02x.%02x%02x       "
				"%-12s  %4d  %s\n", 
				mac->addr[0], mac->addr[1], mac->addr[2],
				mac->addr[3], mac->addr[4], mac->addr[5],
			    (mac->type)? "Static" : "Dynamic",
				mac->vlan,
				name ? name : "N/A"
				);
	}
}

int buf_alloc_swcfgr(struct swcfgreq *swcfgr, int sock_fd)
{
	void *buf;
	int page_size = sysconf(_SC_PAGE_SIZE);
	int size = page_size;
	int status;

	buf = malloc(size);
	if (buf == NULL)
		return -ENOMEM;

	do {
		swcfgr->buf.size = size;
		swcfgr->buf.addr = buf;
		status = ioctl(sock_fd, SIOCSWCFG, swcfgr);
		if (status >= 0)
			return status;

		if (errno != ENOMEM)
			return -errno;

		size += page_size;
		buf = realloc(buf, size);
		if (buf == NULL)
			return -ENOMEM;
	} while (1);
}

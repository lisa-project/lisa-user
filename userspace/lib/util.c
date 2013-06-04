#ifdef LiSA
	#include <linux/net_switch.h>
#endif
#include <linux/sockios.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
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

int parse_mac(const char *str, unsigned char *mac)
{
	int a, b, c, n;

	if (sscanf(str, "%x.%x.%x%n", &a, &b, &c, &n) != 3)
		return -EINVAL;
	if (strlen(str) != n)
		return -EINVAL;

	mac[0] = (a & 0xff00) >> 8;
	mac[1] = (a & 0x00ff) >> 0;
	mac[2] = (b & 0xff00) >> 8;
	mac[3] = (b & 0x00ff) >> 0;
	mac[4] = (c & 0xff00) >> 8;
	mac[5] = (c & 0x00ff) >> 0;

	return 0;
}


void print_mac_list(FILE *out, struct list_head *macs,
		char *(*get_if_name)(int, void*), void *priv)
{
	struct net_switch_mac_e *mac;
	fprintf(out,
		"Destination Address  Address Type  VLAN  Destination Port\n"
		"-------------------  ------------  ----  ----------------\n");
	list_for_each_entry(mac, macs, lh) {
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

void print_mac(FILE *out, void *buf, int size, char *(*get_if_name)(int, void*), void *priv)
{
	struct net_switch_mac_e *mac, *end =
		(struct net_switch_mac_e *)((char *)buf + size);

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

int read_key(void) {
	int ret;
	struct termios t_old, t_new;

	tcgetattr(0, &t_old);
	t_new = t_old;
	t_new.c_lflag = ~ICANON;
	t_new.c_cc[VTIME] = 0;
	t_new.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &t_new);
	ret = getchar();
	tcsetattr(0, TCSANOW, &t_old);
	return ret;
}

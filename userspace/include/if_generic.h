#ifndef _IF_GENERIC_H
#define _IF_GENERIC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

static __inline__ int if_name_generic(char *name, const char *type, const char *num) {
	int status;
	int n = atoi(num);

	/* convert forth and back to int to avoid 0-left-padded numbers */
	status = snprintf(name, IFNAMSIZ, "%s%d", type, n);

	/* use assert() since number range is validated through cli */
	assert(status < IFNAMSIZ);

	return n;
}
#define if_name_ethernet(name, num) if_name_generic(name, "eth", num)
#define if_name_vlan(name, num) if_name_generic(name, "vlan", num)

int if_parse_generic(const char *name, const char *type);
#define if_parse_vlan(name) if_parse_generic(name, "vlan");

int if_get_index(const char *name, int sock_fd);

#endif

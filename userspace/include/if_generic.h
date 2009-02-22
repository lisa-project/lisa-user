#ifndef _IF_GENERIC_H
#define _IF_GENERIC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

#include "netlink.h"
#include "list.h"

/* Build a linux netdevice name with a generic structure of "type"
 * immediately followed by "numeric identifier" (e.g. "eth1").
 *
 * name must point to a buffer at least IFNAMSIZ long; type is the
 * netdevice type, and num is the string representation of the
 * numeric identifier. Contents of num must be externally validated
 * as an integer before calling this function.
 */
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

/* Parse a linux netdevice name with a generic structure of "type"
 * immediately followed by "numeric identifier" (e.g. "eth1").
 * 
 * name is the input string to be parsed and type is the netdevice
 * type. On successful completion, the numeric identifier is
 * returned. On error, -1 is returned.
 */
int if_parse_generic(const char *name, const char *type);
#define if_parse_vlan(name) if_parse_generic(name, "vlan");

int if_get_index(const char *name, int sock_fd);

char *if_get_name(int if_index, int sock_fd, char *name);

struct if_addr {
	struct list_head lh;
	int ifindex;

	/* address family */
	int af;

	/* ipv4 address and mask length */
	struct in_addr inet;
	int prefixlen;
};

int if_get_addr(int ifindex, int af, struct list_head *lh, struct rtnl_handle *__rth);

static __inline__ int ip_addr_overlap(struct in_addr a1, int m1, struct in_addr a2, int m2) {
	int off = 32 - (m1 < m2 ? m1 : m2);

	return (ntohl(a1.s_addr) >> off) == (ntohl(a2.s_addr) >> off);
}

int if_change_addr(int cmd, int ifindex, struct in_addr addr, int prefixlen, int secondary, struct rtnl_handle *rth);

#endif

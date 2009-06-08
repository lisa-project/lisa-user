#ifndef _IF_GENERIC_H
#define _IF_GENERIC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

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
#define if_name_channel(name, num) if_name_generic(name, "po", num)

/* Parse a linux netdevice name with a generic structure of "type"
 * immediately followed by "numeric identifier" (e.g. "eth1").
 * 
 * name is the input string to be parsed and type is the netdevice
 * type. On successful completion, the numeric identifier is
 * returned. On error, -1 is returned.
 */
int if_parse_generic(const char *name, const char *type);
#define if_parse_ethernet(name) if_parse_generic(name, "eth")
#define if_parse_vlan(name) if_parse_generic(name, "vlan")

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

#define IF_MAP_HASH_SIZE 17

struct if_map_hash_entry {
	struct list_head lh;
	struct net_switch_dev *dev;
};

struct if_map {
	/* number of (net)devices in map */
	int size;

	/* array of (net)devices */
	struct net_switch_dev *dev;

	/* ifindex hash of (net)devices */
	struct list_head ifindex_hash[IF_MAP_HASH_SIZE];

	/* local cache for if_get_name fallback */
	struct net_switch_dev cache;

	/* local cache for normal operation */
	struct net_switch_dev *last_dev;
};

#define if_map_ifindex_hash(i) ((i) % IF_MAP_HASH_SIZE)

static __inline__ void if_map_init(struct if_map *map)
{
	memset(map, 0, sizeof(struct if_map));
}

struct net_switch_dev *if_map_lookup_ifindex(struct if_map *map, int ifindex, int sock_fd);
int if_map_init_ifindex_hash(struct if_map *map);
int if_map_fetch(struct if_map *map, int type, int sock_fd);
int if_settings_cmd(int ifindex, int cmd, int sock_fd, struct ethtool_cmd *settings);

#define if_get_settings(ifindex, sock_fd, settings) if_settings_cmd(ifindex, ETHTOOL_GSET, sock_fd, settings)
#define if_set_settings(ifindex, sock_fd, settings) if_settings_cmd(ifindex, ETHTOOL_SSET, sock_fd, settings)

struct if_map_priv {
	struct if_map *map;
	int sock_fd;
};

static __inline__ char *if_map_print_mac(int ifindex, void *__priv)
{
	struct if_map_priv *priv = __priv;
	struct net_switch_dev *dev = if_map_lookup_ifindex(priv->map, ifindex, priv->sock_fd);

	return dev == NULL ? NULL : dev->name;
}

static __inline__ void if_map_cleanup(struct if_map *map)
{
	int i;
	struct if_map_hash_entry *he, *tmp;

	if (map->ifindex_hash[0].prev != NULL)
		for (i = 0; i < IF_MAP_HASH_SIZE; i++)
			list_for_each_entry_safe(he, tmp, &map->ifindex_hash[i], lh)
				free(he);
}

#endif

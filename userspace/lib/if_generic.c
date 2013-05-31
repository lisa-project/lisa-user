// define _GNU_SOURCE supresses the warning for asprintf
#define _GNU_SOURCE
#include <sys/ioctl.h>

#include "if_generic.h"
#include "util.h"

int if_parse_generic(const char *name, const char *type)
{
	int ret, n;
	char *fmt;
	size_t len;

	fmt = alloca((len = strlen(type)) + 5);
	strcpy(fmt, type);
	strcpy(fmt + len, "%d%n");

	if (!sscanf(name, fmt, &ret, &n))
		return -1;

	if (strlen(name) != n)
		return -1;

	return ret;
}

int if_get_index(const char *name, int sock_fd)
{
	struct ifreq ifr;
	int ret = 0;

	if (strlen(name) >= IFNAMSIZ)
		return 0;

	strcpy(ifr.ifr_name, name);

	assert(sock_fd != -1);

	if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr))
		ret = ifr.ifr_ifindex;

	return ret;
}

char *canonical_if_name(struct net_switch_device *nsdev)
{
	char *ret = NULL;
	int n, status = -1;

	if (nsdev == NULL)
		return NULL;

	switch (nsdev->type) {
	case IF_TYPE_SWITCHED:
	case IF_TYPE_ROUTED:
		if ((n = if_parse_ethernet(nsdev->name)) >= 0)
			status = asprintf(&ret, "Ethernet %d", n);
		else
			status = asprintf(&ret, "netdev %s", nsdev->name);
		break;
	case IF_TYPE_VIF:
		status = asprintf(&ret, "vlan %d", nsdev->vlan);
		break;
	}

	return status == -1 ? NULL : ret;
}

char *short_if_name(struct net_switch_device *nsdev)
{
	char *ret = NULL;
	int n, status = -1;

	if (nsdev == NULL)
		return NULL;

	switch (nsdev->type) {
	case IF_TYPE_SWITCHED:
	case IF_TYPE_ROUTED:
		if ((n = if_parse_ethernet(nsdev->name)) >= 0)
			status = asprintf(&ret, "Et%d", n);
		else
			status = asprintf(&ret, "net %s", nsdev->name);
		break;
	case IF_TYPE_VIF:
		status = asprintf(&ret, "vlan %d", nsdev->vlan);
		break;
	}

	return status == -1 ? NULL : ret;
}



char *if_get_name(int if_index, int sock_fd, char *name)
{
	struct ifreq ifr;

	assert(sock_fd != -1);

	ifr.ifr_ifindex = if_index;

	if (ioctl(sock_fd, SIOCGIFNAME, &ifr))
		return NULL;

	if (!name)
		return strdup(ifr.ifr_name);

	strncpy(name, ifr.ifr_name, IFNAMSIZ);	

	return name;
}

struct build_ip_list_priv {
	struct list_head *lh;
	int ifindex;
};

static int if_build_ip_list(const struct sockaddr_nl *who, const struct nlmsghdr *n, 
		void *__arg)
{
	struct ifaddrmsg *ifa = NLMSG_DATA(n);
	struct rtattr *rta_tb[IFA_MAX+1];
	struct build_ip_list_priv *arg = __arg;

	if (n->nlmsg_type != RTM_NEWADDR)
		return 0;
	if (n->nlmsg_len < NLMSG_LENGTH(sizeof(*ifa)))
		return 0;

	if (arg->ifindex && arg->ifindex != ifa->ifa_index)
		return 0;

	parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa), n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));

	// FIXME for now all interfaces seem to have the same address in both
	// fields (IFA_LOCAL and IFA_ADDRESS)
	if (rta_tb[IFA_LOCAL]) {
		struct if_addr *addr;

		addr = malloc(sizeof(*addr));
		if (addr == NULL)
			return 0;

		addr->ifindex = ifa->ifa_index;
		addr->af = ifa->ifa_family;
		addr->inet = *(struct in_addr *)RTA_DATA(rta_tb[IFA_LOCAL]);
		addr->prefixlen = ifa->ifa_prefixlen;

		list_add_tail(&addr->lh, arg->lh);
	}

	return 0;
}

int if_get_addr(int ifindex, int af, struct list_head *lh, struct rtnl_handle *rth)
{
	struct rtnl_handle __rth;
	struct build_ip_list_priv priv = {
		.ifindex = ifindex,
		.lh = lh
	};

	if (rth == NULL && rtnl_open(rth = &__rth))
		return -1;

	// FIXME we must check if passing af to rtnl_wilddump_request
	// is safe and meaningful
	if (rtnl_wilddump_request(rth, af, RTM_GETADDR) < 0) {
		perror("Cannot send dump request"); // FIXME output
		return -1;
	}

	if (rtnl_dump_filter(rth, if_build_ip_list, &priv) < 0) {
		fprintf(stderr, "Dump terminated\n"); // FIXME output
		return -1;
	} 

	if (rth == &__rth)
		rtnl_close(rth);

	return 0;
}

int if_change_addr(int cmd, int ifindex, struct in_addr addr, int prefixlen, int secondary, struct rtnl_handle *rth)
{
	struct rtnl_handle __rth;
	struct {
		struct nlmsghdr 	n;
		struct ifaddrmsg 	ifa;
		char   				buf[256];
	} req;
	int ret;

	if (rth == NULL && rtnl_open(rth = &__rth))
		return -1;
	
	/* Initialize request argument */
	memset(&req, 0, sizeof(req));
	
	/* setup struct nlmsghdr */
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = cmd;

	/* setup struct ifaddrmsg */
	req.ifa.ifa_index = ifindex;
	req.ifa.ifa_family = AF_INET;
	if (secondary)
		req.ifa.ifa_flags |= IFA_F_SECONDARY;
	req.ifa.ifa_prefixlen = prefixlen;
	req.ifa.ifa_scope = *(uint8_t *)&addr.s_addr == 127 ? RT_SCOPE_HOST : 0;
	
	/* Complete request with the parsed inet_prefix */
	addattr_l(&req.n, sizeof(req), IFA_LOCAL, &addr.s_addr, prefixlen);
	addattr_l(&req.n, sizeof(req), IFA_ADDRESS, &addr.s_addr, prefixlen);
	
	/* Make request */
	if ((ret = rtnl_talk(rth, &req.n)) < 0)
		return ret;
	
	/* Close netlink socket */	
	if (rth == &__rth)
		rtnl_close(rth);

	return 0;
}

struct net_switch_device *if_map_lookup_ifindex(struct if_map *map, int ifindex, int sock_fd)
{
	struct net_switch_device *dev;

	if (map->ifindex_hash[0].prev == NULL) {
		if (map->cache.ifindex == ifindex)
			return map->cache.name[0] == '\0' ? NULL : &map->cache;
		if (if_get_name(ifindex, sock_fd, map->cache.name) == NULL) {
			map->cache.name[0] = '\0';
			return NULL;
		}
		map->cache.ifindex = ifindex;
		return &map->cache;
	}

	if (map->last_dev && map->last_dev->ifindex == ifindex)
		return map->last_dev;

	list_for_each_entry(dev, &map->ifindex_hash[if_map_ifindex_hash(ifindex)], lh) {
		if (dev->ifindex == ifindex) {
			map->last_dev = dev;
			return dev;
		}
	}

	return NULL;
}

int if_map_init_ifindex_hash(struct if_map *map)
{
	int i;
	struct net_switch_device *dev, *he;
	
	for (i = 0; i < IF_MAP_HASH_SIZE; i++)
		INIT_LIST_HEAD(&map->ifindex_hash[i]);

	list_for_each_entry(dev, &map->dev, lh) {
		he = malloc(sizeof(*he));
		if (he == NULL)
			return -1;
		memcpy(he, dev, sizeof(struct net_switch_device));
		list_add_tail(&he->lh, &map->ifindex_hash[if_map_ifindex_hash(dev->ifindex)]);
	}

	return 0;
}

int if_map_fetch(struct if_map *map, int type)
{
	return sw_ops->get_if_list(sw_ops, type, &map->dev);
}

int if_settings_cmd(int ifindex, int cmd, int sock_fd, struct ethtool_cmd *settings)
{
	struct ifreq ifr;

	assert(sock_fd != -1);
	assert(settings != NULL);

	if (cmd != ETHTOOL_GSET && cmd != ETHTOOL_SSET)
		return -EINVAL;

	memset(&ifr, 0, sizeof(ifr));
	if_get_name(ifindex, sock_fd, ifr.ifr_name);
	ifr.ifr_data = settings;
	settings->cmd = cmd;
	return ioctl(sock_fd, SIOCETHTOOL, &ifr);
}

int if_get_flags(int ifindex, int sock_fd, int *flags)
{
	int ret = 0;
	struct ifreq ifr;

	assert(sock_fd != -1);
	memset(&ifr, 0, sizeof(ifr));

	/* Get the name of the interface */
	if_get_name(ifindex, sock_fd, ifr.ifr_name);

	ret = ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
	if (ret)
		return ret;

	*flags = ifr.ifr_flags;
	return ret;
}

int if_set_flags(int ifindex, int sock_fd, int flags)
{
	struct ifreq ifr;

	assert(sock_fd != -1);
	memset(&ifr, 0, sizeof(ifr));

	/* Get the name of the interface */
	if_get_name(ifindex, sock_fd, ifr.ifr_name);

	ifr.ifr_flags = flags;
	return ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
}

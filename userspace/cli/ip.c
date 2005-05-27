#include "ip.h"

static struct idxmap *idxmap[16];

/* Returns device index  for device represented by name 
 (if found in idxmap) and caches name and device index */
int index_by_if_name(const char *name) {
	static char ncache[16];
	static int icache;
	struct idxmap *im;
	int i;

	if (name == NULL)
		return 0;
	if (icache && strcmp(name, ncache) == 0)
		return icache;
	for (i=0; i<16; i++) {
		for (im = idxmap[i]; im; im = im->next) {
			if (strcmp(im->name, name) == 0) {
				icache = im->index;
				strcpy(ncache, name);
				return im->index;
			}
		}
	}
	return 0;
}

/* Returns device name for a specified device index 
 (if found in idxmap). Otherwise return if<idx> */
char *if_name_by_index(int idx) {
	static char buf[16];
	struct idxmap *im;

	for (im = idxmap[idx]; im; im = im->next)	
		if (im->index == idx)
			return im->name;
	snprintf(buf, 16, "if%d", idx);
	return buf;
}

/* store interface info in idxmap */
int ll_remember_index(const struct sockaddr_nl *who, const struct nlmsghdr *n, void *arg) {
	int h;
	struct ifinfomsg *ifi = NLMSG_DATA(n);
	struct idxmap *im, **imp;
	struct rtattr *tb[IFLA_MAX+1];

	if (n->nlmsg_type != RTM_NEWLINK)
		return 0;

	if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifi)))
		return -1;


	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), IFLA_PAYLOAD(n));
	if (tb[IFLA_IFNAME] == NULL)
		return 0;

	h = ifi->ifi_index&0xF;

	for (imp=&idxmap[h]; (im=*imp)!=NULL; imp = &im->next)
		if (im->index == ifi->ifi_index)
			break;

	if (im == NULL) {
		im = malloc(sizeof(*im));
		if (im == NULL)
			return 0;
		im->next = *imp;
		im->index = ifi->ifi_index;
		*imp = im;
	}

	im->type = ifi->ifi_type;
	im->flags = ifi->ifi_flags;
	if (tb[IFLA_ADDRESS]) {
		int alen;
		im->alen = alen = RTA_PAYLOAD(tb[IFLA_ADDRESS]);
		if (alen > sizeof(im->addr))
			alen = sizeof(im->addr);
		memcpy(im->addr, RTA_DATA(tb[IFLA_ADDRESS]), alen);
	} else {
		im->alen = 0;
		memset(im->addr, 0, sizeof(im->addr));
	}
	strcpy(im->name, RTA_DATA(tb[IFLA_IFNAME]));
	return 0;
}

static int store_nlmsg(const struct sockaddr_nl *who, const struct nlmsghdr *n, 
		       void *arg) {
	struct nlmsg_list **linfo = (struct nlmsg_list**)arg;
	struct nlmsg_list *h;
	struct nlmsg_list **lp;

	h = malloc(n->nlmsg_len+sizeof(void*));
	if (h == NULL)
		return -1;

	memcpy(&h->h, n, n->nlmsg_len);
	h->next = NULL;

	for (lp = linfo; *lp; lp = &(*lp)->next) /* NOTHING */;
	*lp = h;

	ll_remember_index(who, n, NULL);
	return 0;
}

/* Initialize netlink communication and fetch idxmap entries */
int ll_init_map(struct rtnl_handle *rth) {
	int ret  = 0;
	
	if ((ret = rtnl_wilddump_request(rth, AF_UNSPEC, RTM_GETLINK)) < 0) {
		perror("Cannot send dump request");
		return ret;
	} 

	if ((ret = rtnl_dump_filter(rth, ll_remember_index, &idxmap)) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return ret;
	}
	return ret;
}
char buf[32];

char *prefix_2_nmask(int bitcount) {
	int off=0, r, c, i;
	unsigned char n;

	memset(buf, 0, sizeof(buf));
	assert(bitcount >= 1 && bitcount <=32);
	c = bitcount / 8; 
	r = bitcount % 8;
	for (i = 0; i < c; i++) {
		off += sprintf(buf+off, "%d.", 255);
	}
	if (c < 4) {
		n = 0xFF << (8-r);
		off += sprintf(buf+off, "%d.", n);
		i++;
	}
	for (c = i; i<4; i++)
		off += sprintf(buf+off, "0.");
	buf[off-1] = '\0';
	return buf;
}

void print_addr_info(struct nlmsghdr *n, struct ifaddrmsg *ifa,
		int flush, struct list_head *ipl) {
	struct rtattr *rta_tb[IFA_MAX+1];
	char abuf[MAX_NAME_LEN];
	char buf[32];
	struct ip_addr_entry *entry;

	assert(n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)) > 0);
	parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa), n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));

	if (!rta_tb[IFA_LOCAL])
		rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
	if (!rta_tb[IFA_ADDRESS])
		rta_tb[IFA_ADDRESS] = rta_tb[IFA_LOCAL];

	if (rta_tb[IFA_LOCAL]) {
		if (!flush) {
			entry = (struct ip_addr_entry *)malloc(sizeof(struct ip_addr_entry));
			if (!entry) {
				perror("Out of memory");
				return;
			}
			inet_ntop(ifa->ifa_family, RTA_DATA(rta_tb[IFA_LOCAL]), entry->inet, 
					MAX_NAME_LEN);
			entry->inet[MAX_NAME_LEN-1] = '\0';
			strncpy(entry->mask, prefix_2_nmask(ifa->ifa_prefixlen), MAX_NAME_LEN);
			entry->mask[MAX_NAME_LEN-1] = '\0';
			list_add_tail(&entry->lh, ipl);
			return;
		}
		/* here we delete the address */
		memset(buf, 0, sizeof(buf));
		inet_ntop(ifa->ifa_family, RTA_DATA(rta_tb[IFA_LOCAL]), abuf, sizeof(abuf));
		abuf[sizeof(abuf)-1] = '\0';
		snprintf(buf, sizeof(buf), "%s/%d", abuf, ifa->ifa_prefixlen);
		buf[sizeof(buf)-1] = '\0';
		change_ip_address(RTM_DELADDR, 
				if_name_by_index(ifa->ifa_index), buf, 0);
	}
}


void print_addr(int ifindex, struct nlmsg_list *ainfo, 
		int flush, struct list_head *ipl) {
	
	for (; ainfo; ainfo = ainfo->next) {
		struct nlmsghdr *n = &ainfo->h;
		struct ifaddrmsg *ifa = NLMSG_DATA(n);

		if (n->nlmsg_type != RTM_NEWADDR)
			continue;
		if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
			return;
		if (ifindex && ifindex != ifa->ifa_index)
			continue;
		print_addr_info(n, ifa, flush, ipl);
	}
}

int change_ip_address(int cmd, char *dev, char *address, int secondary) {
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr 	n;
		struct ifaddrmsg 	ifa;
		char   			buf[256];
	} req;
	inet_prefix lcl, peer;
	int ret;
	
	memset(&req, 0, sizeof(req));
	
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = cmd; /* add ip address */
	req.ifa.ifa_family = AF_INET;
	if (secondary)
		req.ifa.ifa_flags |= IFA_F_SECONDARY;

	/* Open netlink socket */
	if ((ret = rtnl_open(&rth)))
		return ret;
	
	/* Parse address into inet_prefix lcl */
	if ((ret = get_prefix(&lcl, address, req.ifa.ifa_family)))
		return ret;
	/* Complete request with the parsed inet_prefix */
	addattr_l(&req.n, sizeof(req), IFA_LOCAL, &lcl.data, lcl.bytelen);
	peer = lcl;
	addattr_l(&req.n, sizeof(req), IFA_ADDRESS, &lcl.data, lcl.bytelen);
	
	if (req.ifa.ifa_prefixlen == 0)
		req.ifa.ifa_prefixlen = lcl.bitlen;
	req.ifa.ifa_scope = default_scope(&lcl);
	
	/* Initialization (gets devices idxmap) */
	if ((ret = ll_init_map(&rth)))
		return ret;
	
	/* Check if the device name is valid */
	if ((req.ifa.ifa_index = index_by_if_name(dev)) == 0) {
		fprintf(stderr, "Cannot find device \"%s\"\n", dev);
		return -1;
	}
	/* Make request */
	if ((ret = rtnl_talk(&rth, &req.n)) < 0)
		return ret;
	
	/* Close netlink socket */	
	rtnl_close(&rth);

	return 0;
}

struct list_head *list_ip_addr(char *dev, int flush) {
	struct nlmsg_list *ainfo = NULL;
	struct rtnl_handle rth;
	int ifindex = 0;
	struct list_head *ip_list = NULL;  
	
	if (rtnl_open(&rth))
		return NULL;
	
	if (ll_init_map(&rth))
		return NULL;
	
	if (dev) {
		if ((ifindex = index_by_if_name(dev)) <= 0) {
			fprintf(stderr, "Device \"%s\" does not exist.\n", dev);
			return NULL;
		}
	}

	if (rtnl_wilddump_request(&rth, AF_INET, RTM_GETADDR) < 0) {
		perror("Cannot send dump request");
		return NULL;
	}

	if (rtnl_dump_filter(&rth, store_nlmsg, &ainfo) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return NULL;
	} 

	ip_list = (struct list_head *) malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(ip_list);
	
	print_addr(ifindex, ainfo, flush, ip_list);

	rtnl_close(&rth);
	return ip_list;
}

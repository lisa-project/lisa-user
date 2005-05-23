#ifndef __SW_NETLINK_H
#define __SW_NETLINK_H

#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct rtnl_handle {
    int         fd;
    struct sockaddr_nl  local;
    struct sockaddr_nl  peer;
    __u32           seq;
    __u32           dump;
};

struct nlmsg_list {
	struct nlmsg_list *next;
	struct nlmsghdr h;
};

typedef struct {
	__u8 family;
	__u8 bytelen;
	__s16 bitlen;
	__u32 data[4];
} inet_prefix;

struct idxmap {
	struct idxmap * next;
	int		index;
	int		type;
	int		alen;
	unsigned	flags;
	unsigned char	addr[8];
	char		name[16];
};

typedef int (*rtnl_filter_t)(const struct sockaddr_nl *, const struct nlmsghdr *n, void *);

/* Exported functions */
extern int default_scope(inet_prefix *);
extern int get_integer(int *, const char *, int);
extern int get_prefix(inet_prefix *, const char *, int);
extern int parse_rtattr(struct rtattr *[], int, struct rtattr *, int);
extern int addattr_l(struct nlmsghdr *, int, int, const void *, int);
extern int rtnl_open(struct rtnl_handle *);
extern int rtnl_wilddump_request(struct rtnl_handle *, int, int);
extern int rtnl_dump_filter(struct rtnl_handle *, rtnl_filter_t, void *);
extern int rtnl_talk(struct rtnl_handle *, struct nlmsghdr *);
extern void rtnl_close(struct rtnl_handle *);

#endif

#ifndef __SW_IP_H
#define __SW_IP_H

#include "netlink.h"
#include "list.h"

#define MAX_NAME_LEN 16

extern int change_ip_address(int, char *, char *, int);
extern struct list_head *list_ip_addr(char *, int);

struct ip_addr_entry {
	struct list_head lh;
	char inet[MAX_NAME_LEN];
	char mask[MAX_NAME_LEN];
};

#endif

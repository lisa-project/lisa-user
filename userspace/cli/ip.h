#ifndef __SW_IP_H
#define __SW_IP_H

#include "netlink.h"

#define FMT_NOCMD 0
#define FMT_CMD 1

extern int change_ip_address(int, char *, char *, int);
extern int list_ip_addr(FILE *, char *, int, int);

#endif

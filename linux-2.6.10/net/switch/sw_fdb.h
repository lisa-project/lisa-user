#ifndef _SW_FDB_H
#define _SW_FDB_H

#include "sw_private.h"

extern void sw_fdb_init(struct net_switch *sw);
extern void fdb_cleanup_port(struct net_switch_port *);

static __inline__ int sw_mac_hash(const unsigned char *mac) {
	unsigned long x;

	x = mac[0];
	x = (x << 2) ^ mac[1];
	x = (x << 2) ^ mac[2];
	x = (x << 2) ^ mac[3];
	x = (x << 2) ^ mac[4];
	x = (x << 2) ^ mac[5];

	x ^= x >> 8;

	return x & (SW_HASH_SIZE - 1);
}

#endif

#ifndef _SW_PRIVATE_H
#define _SW_PRIVATE_H

#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>


#define SW_HASH_SIZE_BITS 12
#define SW_HASH_SIZE (1 << SW_HASH_SIZE_BITS)

/* Hash bucket */
struct net_switch_bucket {
	/*
		List of fdb_entries
	*/
	struct list_head head;
	/*
		Mutex for operations on the list
		associated with this bucket.
	*/
	struct semaphore mutex;
};

struct net_switch {
	/* List of all ports in the switch */
	struct list_head ports;

	/* We don't want an interface to be added or removed "twice". This
	   would mess up the usage counter, the promiscuity counter and many
	   other things.
	 */
	struct semaphore adddelif_mutex;
	
	/*
		Switch forwarding database (hashtable)
	*/
	struct net_switch_bucket hash[SW_HASH_SIZE];
};

struct net_switch_port {
	/* Linking with other ports in a list */
	struct list_head lh;

	/* Physical device associated with this port */
	struct net_device *dev;

	/* Ports are freed with an RCU callback
	 */
	struct rcu_head rcu;
};

/* Hash Entry */
struct net_switch_fdb_entry {
	unsigned char mac[6];
	unsigned char vlan_id[2];	
	struct net_switch_port *port;
	struct list_head next;
};

#endif

#ifndef _SW_PRIVATE_H
#define _SW_PRIVATE_H

#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>

struct net_switch {
	/* List of all ports in the switch */
	struct list_head ports;

	/* We don't want an interface to be added or removed "twice". This
	   would mess up the usage counter, the promiscuity counter and many
	   other things.
	 */
	struct semaphore adddelif_mutex;
};

struct net_switch_port {
	/* Linking with other ports in a list */
	struct list_head lh;

	/* Physical device associated with this port */
	struct net_device *dev;

	/* When deleting interfaces, we have to wait until everybody's done
	   with the corresponding port.
	 */
	wait_queue_head_t cleanup_q;

	/* Ports are freed with an RCU callback
	 */
	struct rcu_head rcu;
};

#endif

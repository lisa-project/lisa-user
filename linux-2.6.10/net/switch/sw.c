#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/if_ether.h>
#include <asm/semaphore.h>

#include "sw_private.h"
#include "sw_debug.h"
#include "sw_fdb.h"
#include "sw_proc.h"

MODULE_DESCRIPTION("Cool stuff");
MODULE_AUTHOR("us");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

extern int (*sw_handle_frame_hook)(struct net_switch_port *p, struct sk_buff **pskb);

static inline void __dump_bitmap(unsigned char *bmp) {
	int i, j;
	char buf[65];

	for(i = 0; i < 16; i++) {
		for(j = 0; j < 32; j++) {
			sprintf(buf + 2 * j, "%02hx", *bmp);
			bmp++;
		}
		dbg("bmp: %s\n", buf);
	}
}

struct net_switch sw;
/* Safely add an interface to the switch
 */

/* Set a forbidden vlan mask to allow the default vlans */
static inline void __sw_allow_default_vlans(unsigned char *forbidden_vlans) {
	sw_allow_vlan(forbidden_vlans, 1);
	sw_allow_vlan(forbidden_vlans, 1002);
	sw_allow_vlan(forbidden_vlans, 1003);
	sw_allow_vlan(forbidden_vlans, 1004);
	sw_allow_vlan(forbidden_vlans, 1005);
}

/* Add an interface to the switch. The switch configuration mutex must
   be acquired from outside.
 */
static int sw_addif(struct net_device *dev) {
	struct net_switch_port *port;
	if(rcu_dereference(dev->sw_port) != NULL) {
		/* dev->sw_port shouldn't be changed elsewhere, so
		   we don't necessarily need rcu_dereference here
		 */
		return -EBUSY;
	}
	if((port = kmalloc(sizeof(struct net_switch_port), GFP_KERNEL)) == NULL)
		return -ENOMEM;
	memset(port, 0, sizeof(struct net_switch_port));
	port->dev = dev;
	port->sw = &sw;
    port->vlan = 1; /* By default all ports are in vlan 1 */
	memset(port->forbidden_vlans, 0xff, 512);
	__sw_allow_default_vlans(port->forbidden_vlans);
	__dump_bitmap(port->forbidden_vlans);
    sw_vdb_add_port(1, port);
	list_add_tail_rcu(&port->lh, &sw.ports);
	rcu_assign_pointer(dev->sw_port, port);
	dev_hold(dev);
	dev_set_promiscuity(dev, 1);
	dbg("Added device %s to switch\n", dev->name);
	return 0;
}

/* Effectively add a port to all allowed vlans in a bitmap of
   forbidden vlans.
 */
static inline void __sw_add_to_vlans(struct net_switch_port *port) {
	int n, vlan = 0;
	unsigned char mask, *bmp = port->forbidden_vlans;
	//__dump_bitmap(port->forbidden_vlans);
	for(n = 0; n < 512; n++, bmp++) {
		for(mask = 1; mask; mask <<= 1, vlan++) {
			if(*bmp & mask)
				continue;
			sw_vdb_add_port(vlan, port);
		}
	}
}

/* Effectively remove a port from all allowed vlans in a bitmap of
   forbidden vlans.
 */
static inline void __sw_remove_from_vlans(struct net_switch_port *port) {
	int n, vlan = 0;
	unsigned char mask, *bmp = port->forbidden_vlans;
	for(n = 0; n < 512; n++, bmp++) {
		for(mask = 1; mask; mask <<= 1, vlan++) {
			if(*bmp & mask)
				continue;
			sw_vdb_del_port(vlan, port);
		}
	}
}

/* Set a port's trunk mode and make appropriate changes to the
   vlan database.
 */
int sw_set_port_trunk(struct net_switch_port *port, int trunk) {
	if (!port)
		return -EINVAL;
	if(port->flags & SW_PFL_TRUNK) {
		if(trunk)
			return -EINVAL;
		__sw_remove_from_vlans(port);
		port->flags &= ~SW_PFL_TRUNK;
		sw_vdb_add_port(port->vlan, port);
	} else {
		if(!trunk)
			return -EINVAL;
		sw_vdb_del_port(port->vlan, port);
		port->flags |= SW_PFL_TRUNK;
		__sw_add_to_vlans(port);
	}
	return 0;
}

/* Change a port's bitmap of forbidden vlans and, if necessary,
   make appropriate changes to the vlan database.
 */
int sw_set_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	unsigned char mask;
	int n, vlan = 0;

	__sw_allow_default_vlans(forbidden_vlans);
	if(port->flags & SW_PFL_TRUNK) {
		for(n = 0; n < 512; n++, old++, new++) {
			for(mask = 1; mask; mask <<= 1, vlan++) {
				if(!((*old ^ *new) & mask))
					continue;
				if(*new & mask)
					sw_vdb_del_port(vlan, port);
				else
					sw_vdb_add_port(vlan, port);
			}
		}
	}
	memcpy(port->forbidden_vlans, forbidden_vlans, 512);
	return 0;
}

/* Update a port's bitmap of forbidden vlans by allowing vlans from a
   given bitmap of forbidden vlans. If necessary, make the appropriate
   changes to the vlan database.
 */
int sw_add_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char bmp[512];
	unsigned char *p = bmp;
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	int n;

	for(n = 0; n < 512; n++, old++, new++, p++)
		*p = *old & *new;
	return sw_set_port_forbidden_vlans(port, bmp);
}

/* Update a port's bitmap of forbidden vlans by disallowing those vlans
   that are allowed by a given bitmap (of forbidden vlans). If necessary,
   make the appropriate changes to the vlan database.
 */
int sw_del_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char bmp[512];
	unsigned char *p = bmp;
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	int n;

	for(n = 0; n < 512; n++, old++, new++, p++)
		*p = *old | ~*new;
	return sw_set_port_forbidden_vlans(port, bmp);
}

/* Change a port's non-trunk vlan and make appropriate changes to the vlan
   database if necessary.
 */
int sw_set_port_vlan(struct net_switch_port *port, int vlan) {
	if(!port)
		return -EINVAL;
	if(port->vlan == vlan)
		return 0;
	if(!(port->flags & SW_PFL_TRUNK)) {
		sw_vdb_del_port(port->vlan, port);
		sw_vdb_add_port(vlan, port);
	}
	port->vlan = vlan;
	return 0;
}

/* Remove an interface from the switch. Appropriate locks must be held
   from outside to ensure that nobody tries to remove the same interface
   at the same time.
 */
static int sw_delif(struct net_device *dev) {
	struct net_switch_port *port;

	if((port = rcu_dereference(dev->sw_port)) == NULL)
		return -EINVAL;

	/* First disable promiscuous mode, so that there be less chances to
	   still receive packets on this port
	 */
	dev_set_promiscuity(dev, -1);
	/* Now let all incoming queue processors know that frames on this port
	   are not handled by the switch anymore.
	 */
	rcu_assign_pointer(dev->sw_port, NULL);
	/* dev->sw_port is now NULL, so no instance of sw_handle_frame() will
	   process incoming packets on this port.

	   However, at the time we changed the pointer there might have been
	   instances of sw_handle_frame() that were processing incoming
	   packets on this port. Frame processing can add entries to the fdb
	   that reference this port, so we have to wait for all running
	   instances to finish.
	 */
	synchronize_kernel();
	/* Now nobody can add references to this port, so we can safely clean
	   up all existing references from the fdb
	 */
	fdb_cleanup_port(port);
	/* Clean up vlan references: if the port was non-trunk, remove it from
	   its single vlan; otherwise use the disallowed vlans bitmap to remove
	   it from all vlans
	 */
	if(port->flags & SW_PFL_TRUNK) {
		__sw_remove_from_vlans(port);
	} else {
		sw_vdb_del_port(port->vlan, port);
	}
	list_del_rcu(&port->lh);
	/* free port memory and release interface */
	kfree(port);
	dev_put(dev);
	dbg("Removed device %s from switch\n", dev->name);
	return 0;
}

/* Initialize everything associated with a switch */
static void init_switch(struct net_switch *sw) {
	INIT_LIST_HEAD(&sw->ports);
	sw_fdb_init(sw);
	init_switch_proc();
	sw_vdb_init(sw);
}

/* Free everything associated with a switch */
static void exit_switch(struct net_switch *sw) {
	struct list_head *pos, *n;
	struct net_switch_port *port;

	/* Remove all interfaces from switch */
	list_for_each_safe(pos, n, &sw->ports) {
		port = list_entry(pos, struct net_switch_port, lh);
		sw_delif(port->dev);
	}
	sw_fdb_exit(sw);
	sw_vdb_exit(sw);
	cleanup_switch_proc();
}

/* FIXME functia e obsolete, dar am pastrat-o ca exemplu pt luat nume din userspace
static inline int __dev_get_by_name_user(void __user *ptr, struct net_device **pdev) {
	char buf[IFNAMSIZ];
	struct net_device *dev;

	if(copy_from_user(buf, ptr, IFNAMSIZ))
		return -EFAULT;
	buf[IFNAMSIZ - 1] = '\0';

	if((dev = dev_get_by_name(buf)) == NULL)
		return -ENODEV;
	*pdev = dev;

	return 0;
}
*/

/* Handle "deviceless" ioctls. These ioctls are not specific to a certain
   device; they control the switching engine as a whole.
 */
static int sw_deviceless_ioctl(unsigned int cmd, void __user *uarg) {
	struct net_device *dev;
	struct net_switch_ioctl_arg arg;
	unsigned char bitmap[SW_VLAN_BMP_NO];
	int err = -EINVAL;

	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if ((err = copy_from_user(&arg, uarg, sizeof(struct sw_user_arg))))
		return -EINVAL;

	if(cmd != SIOCSWCFG)
		return -EINVAL;

	memset(bitmap, 0xFF, SW_VLAN_BMP_NO);

	/* FIXME zona indicata de arg.name trebuie copiata din userspace */
	switch(arg.cmd) {
	case SWCFG_ADDIF:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_addif(dev);
		dev_put(dev);
		break;
		
	case SWCFG_DELIF:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_delif(dev);
		dev_put(dev);
		break;
	
	case SWCFG_ADDVLAN:
		err = sw_vdb_add_vlan(&sw, arg.vlan, arg.name);
		break;
	case SWCFG_DELVLAN:
		err = sw_vdb_del_vlan(&sw, arg.vlan);
		break;
	case SWCFG_RENAMEVLAN:
		err = sw_vdb_set_vlan_name(&sw, arg.vlan, arg.name);
		break;
	case SWCFG_ADDVLANPORT:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		sw_allow_vlan(bitmap, arg.vlan);
		err = sw_add_port_forbidden_vlans(rcu_dereference(dev->sw_port), bitmap);
		break;
	case SWCFG_DELVLANPORT:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		/* use sw_allow_vlan here because sw_del_port_forbidden_vlans
		   negates the mask
		 */
		sw_allow_vlan(bitmap, arg.vlan);	
		err = sw_del_port_forbidden_vlans(rcu_dereference(dev->sw_port), bitmap);
		break;
	case SWCFG_SETTRUNK:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_set_port_trunk(rcu_dereference(dev->sw_port), arg.vlan);	
		break;
	case SWCFG_SETPORTVLAN:	
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_set_port_vlan(rcu_dereference(dev->sw_port), arg.vlan);	
		break;
	}

	return err;
}

static void dump_packet(const struct sk_buff *skb) {
	int i;
	
	printk(KERN_DEBUG "sw_handle_frame on %s: proto=0x%hx "
			"head=0x%p data=0x%p tail=0x%p end=0x%p\n",
			skb->dev->name, ntohs(skb->protocol),
			skb->head, skb->data, skb->tail, skb->end);
	printk("MAC dump: ");
	for(i = 0; i < skb->mac_len; i++)
		printk("0x%x ", skb->mac.raw[i]);
	printk("\nDATA dump: ");
	for(i = 0; i < 4; i++)
		printk("0x%x ", skb->data[i]);
	printk("\n");
}

/* Handle a frame received on a physical interface */
static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;
	struct skb_extra skb_e;

	if(skb->protocol == ntohs(ETH_P_8021Q)) {
		skb_e.vlan = ntohs(*(unsigned short *)skb->data) & 4095;
		skb_e.has_vlan_tag = 1;
	} else {
		skb_e.vlan = port->vlan;
		skb_e.has_vlan_tag = 0;
	}

	/* Perform some sanity checks */
	if((port->flags & SW_PFL_TRUNK) && !skb_e.has_vlan_tag) {
		dbg("Received untagged frame on TRUNK port %s\n", port->dev->name);
		return 1;
	}
	if(!(port->flags & SW_PFL_TRUNK) && skb_e.has_vlan_tag) {
		dbg("Received tagged frame on non-TRUNK port %s\n", port->dev->name);
		return 1;
	}

	/* If interface is in trunk, check if the vlan is allowed */
	if((port->flags & SW_PFL_TRUNK) &&
			(port->forbidden_vlans[skb_e.vlan / 8] & (1 << (skb_e.vlan % 8)))) {
		dbg("Received frame on vlan %d, which is forbidden on %s\n",
				skb_e.vlan, port->dev->name);
		return 1;
	}

	/* Update the fdb */
	fdb_learn(skb->mac.raw + 6, port, skb_e.vlan);

	dump_packet(skb);

	return 0;
}

/* Module initialization */
static int switch_init(void) {
	init_switch(&sw);
	swioctl_set(sw_deviceless_ioctl);
	sw_handle_frame_hook = sw_handle_frame;
	dbg("Switch module initialized\n");
	return 0;
}

/* Module cleanup */
static void switch_exit(void) {
	exit_switch(&sw);
	swioctl_set(NULL);
	sw_handle_frame_hook = NULL;
	dbg("Switch module unloaded\n");
}

module_init(switch_init);
module_exit(switch_exit);

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <asm/semaphore.h>

#include "sw_private.h"
#include "sw_debug.h"
#include "sw_fdb.h"

MODULE_DESCRIPTION("Cool stuff");
MODULE_AUTHOR("us");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

extern int (*sw_handle_frame_hook)(struct net_switch_port *p, struct sk_buff **pskb);

static struct net_switch sw;

static void init_switch(struct net_switch *sw) {
	INIT_LIST_HEAD(&sw->ports);
	init_MUTEX(&sw->adddelif_mutex);
	sw_fdb_init(sw);
}

/* Safely add an interface to the switch
 */
static int sw_addif(struct net_device *dev) {
	struct net_switch_port *port;
	if(down_interruptible(&sw.adddelif_mutex))
		return -EINTR;
	if(rcu_dereference(dev->sw_port) != NULL) {
		/* dev->sw_port shouldn't be changed elsewhere, so
		   we don't necessarily need rcu_dereference here
		 */
		up(&sw.adddelif_mutex);
		return -EBUSY;
	}
	if((port = kmalloc(sizeof(struct net_switch_port), GFP_KERNEL)) == NULL)
		return -ENOMEM;
	port->dev = dev;
	list_add_tail_rcu(&port->lh, &sw.ports);
	rcu_assign_pointer(dev->sw_port, port);
	dev_hold(dev);
	dev_set_promiscuity(dev, 1);
	dbg("Added device %s to switch\n", dev->name);
	up(&sw.adddelif_mutex);
	return 0;
}

/* Remove an interface from the switch. Appropriate locks must be held
   from outside to ensure that nobody tries to remove the same interface
   at the same time.
 */
static void __sw_delif(struct net_device *dev, struct net_switch_port *port) {
	/* First disable promiscuous mode, so that there be less chances to
	   still receive packets on this port
	 */
	dev_set_promiscuity(dev, -1);
	/* Now let all incoming queue processors know that frames on this port
	   are not handled by the switch anymore.
	 */
	rcu_assign_pointer(dev->sw_port, NULL);
	list_del_rcu(&port->lh);
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
	   up all existing references and finally free the port memory
	 */
	fdb_cleanup_port(port);
	/* free port memory and release interface */
	kfree(port);
	dev_put(dev);
	dbg("Removed device %s from switch\n", dev->name);
}

/* Safely remove an interface from the switch */
static int sw_delif(struct net_device *dev) {
	struct net_switch_port *port;
	if(down_interruptible(&sw.adddelif_mutex))
		return -EINTR;
	if((port = rcu_dereference(dev->sw_port)) == NULL) {
		up(&sw.adddelif_mutex);
		return -EINVAL;
	}
	__sw_delif(dev, port);
	up(&sw.adddelif_mutex);
	return 0;
}

/* Handle "deviceless" ioctls. These ioctls are not specific to a certain
   device; they control the switching engine as a whole.
 */
static int sw_deviceless_ioctl(unsigned int cmd, void __user *uarg) {
	struct net_device *dev;
	char buf[IFNAMSIZ];
	int err = 0;

	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if(copy_from_user(buf, uarg, IFNAMSIZ))
		return -EFAULT;
	buf[IFNAMSIZ - 1] = '\0';

	if((dev = dev_get_by_name(buf)) == NULL)
		return -ENODEV;

	switch(cmd) {
	case SIOCSWADDIF:
		err = sw_addif(dev);
		dev_put(dev);
		return err;
	case SIOCSWDELIF:
		err = sw_delif(dev);
		dev_put(dev);
		return err;
	}
	return -EINVAL;
}

/* Handle a frame received on a physical interface */
static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;
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
	struct list_head *pos, *n;
	struct net_switch_port *port;
	/* Remove all interfaces from switch */
	down_interruptible(&sw.adddelif_mutex);
	list_for_each_safe(pos, n, &sw.ports) {
		port = list_entry(pos, struct net_switch_port, lh);
		__sw_delif(port->dev, port);
	}
	up(&sw.adddelif_mutex);
	/* Interfaces cannot be added now although the lock is released. 
	   To add an interface one must do an ioctl, which calls
	   request_module(). This fails if the module is unloading.
	 */
	swioctl_set(NULL);
	sw_handle_frame_hook = NULL;
	dbg("Switch module unloaded\n");
}

module_init(switch_init);
module_exit(switch_exit);

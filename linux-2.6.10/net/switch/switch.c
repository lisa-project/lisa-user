#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <asm/semaphore.h>

#include "sw_private.h"
#include "sw_debug.h"

MODULE_DESCRIPTION("Cool stuff");
MODULE_AUTHOR("us");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

/* TODO
   + in 2.6 nu mai am MOD_INC_USE_COUNT si MOD_DEC_USE_COUNT; ar fi bine sa
     nu dau jos modulul in timp ce se intampla kestii cu interfetele
 */

extern int (*sw_handle_frame_hook)(struct net_switch_port *p, struct sk_buff **pskb);

static struct net_switch sw;

static void init_switch(struct net_switch *sw) {
	INIT_LIST_HEAD(&sw->ports);
	init_MUTEX(&sw->adddelif_mutex);
}

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
	list_add_tail(&port->lh, &sw.ports);
	rcu_assign_pointer(dev->sw_port, port);
	dev_hold(dev);
	dev_set_promiscuity(dev, 1);
	dbg("Added device %s to switch\n", dev->name);
	up(&sw.adddelif_mutex);
	return 0;
}

static void __sw_delif(struct net_device *dev, struct net_switch_port *port) {
	rcu_assign_pointer(dev->sw_port, NULL);
	list_del(&port->lh);
	kfree(port);
	dev_put(dev);
	dev_set_promiscuity(dev, -1);
	dbg("Removed device %s from switch\n", dev->name);
}

static int sw_delif(struct net_device *dev) {
	struct net_switch_port *port;
	if(down_interruptible(&sw.adddelif_mutex))
		return -EINTR;
	if((port = rcu_dereference(dev->sw_port)) == NULL) {
		up(&sw.adddelif_mutex);
		return -ENODEV;
	}
	__sw_delif(dev, port);
	up(&sw.adddelif_mutex);
	return 0;
}

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
		return -EINVAL;

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

static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;

	printk(KERN_DEBUG "sw_handle_frame on %s\n", skb->dev->name);
	return 0;
}

static int switch_init(void) {
	init_switch(&sw);
	swioctl_set(sw_deviceless_ioctl);
	sw_handle_frame_hook = sw_handle_frame;
	dbg("Switch module initialized\n");
	return 0;
}

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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/netdevice.h>
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

/* We don't want an interface to be added or removed "twice". This
   would mess up the usage counter, the promiscuity counter and many
   other things.
 */
DECLARE_MUTEX(adddelif_mutex);

static int sw_addif(struct net_device *dev) {
	if(down_interruptible(&adddelif_mutex))
		return -EINTR;
	if(rcu_dereference(dev->sw_port) != NULL) {
		/* dev->sw_port shouldn't be changed elsewhere, so
		   we don't necessarily need rcu_dereference here
		 */
		up(&adddelif_mutex);
		return -EBUSY;
	}
	rcu_assign_pointer(dev->sw_port, 1);
	up(&adddelif_mutex);
	dev_hold(dev);
	dev_set_promiscuity(dev, 1);
	dbg("Added device %s to switch\n", dev->name);
	return 0;
}

static int sw_delif(struct net_device *dev) {
	if(down_interruptible(&adddelif_mutex))
		return -EINTR;
	if(rcu_dereference(dev->sw_port) == NULL) {
		up(&adddelif_mutex);
		return -ENODEV;
	}
	rcu_assign_pointer(dev->sw_port, NULL);
	up(&adddelif_mutex);
	dev_put(dev);
	dev_set_promiscuity(dev, -1);
	dbg("Removed device %s from switch\n", dev->name);
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
	swioctl_set(sw_deviceless_ioctl);
	sw_handle_frame_hook = sw_handle_frame;
	dbg("Switch module initialized\n");
	return 0;
}

static void switch_exit(void) {
	swioctl_set(NULL);
	sw_handle_frame_hook = NULL;
	dbg("Switch module unloaded\n");
}

module_init(switch_init);
module_exit(switch_exit);

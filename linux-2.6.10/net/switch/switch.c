#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/netdevice.h>

MODULE_DESCRIPTION("Cool stuff");
MODULE_AUTHOR("us");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

extern int (*sw_handle_frame_hook)(struct net_switch_port *p, struct sk_buff **pskb);

static int sw_addif(struct net_device *dev) {
	rcu_assign_pointer(dev->sw_port, 1);
	dev_hold(dev);
	return 0;
}

static int sw_delif(struct net_device *dev) {
	rcu_assign_pointer(dev->sw_port, NULL);
	dev_put(dev);
	return 0;
}

static int sw_deviceless_ioctl(unsigned int cmd, void __user *uarg) {
	struct net_device *dev;
	char buf[IFNAMSIZ];

	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if(copy_from_user(buf, uarg, IFNAMSIZ))
		return -EFAULT;
	buf[IFNAMSIZ - 1] = '\0';

	if((dev = dev_get_by_name(buf)) == NULL)
		return -EINVAL;

	switch(cmd) {
	case SIOCSWADDIF:
		sw_addif(dev);
		dev_put(dev);
		return 0;
	case SIOCSWDELIF:
		sw_delif(dev);
		dev_put(dev);
		return 0;
	}
	return -EINVAL;
}

static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;

	printk(KERN_DEBUG "sw_handle_frame on %s\n", skb->dev->name);
	return 0;
}

static int switch_init(void) {
	printk("Hello, world!\n");
	swioctl_set(sw_deviceless_ioctl);
	sw_handle_frame_hook = sw_handle_frame;
	return 0;
}

static void switch_exit(void) {
	swioctl_set(NULL);
	sw_handle_frame_hook = NULL;
}

module_init(switch_init);
module_exit(switch_exit);

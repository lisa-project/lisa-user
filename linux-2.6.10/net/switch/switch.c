#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/netdevice.h>

MODULE_DESCRIPTION("Cool stuff");
MODULE_AUTHOR("us");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

extern int (*sw_handle_frame_hook)(struct net_switch_port *p, struct sk_buff **pskb);

static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	printk(KERN_DEBUG "sw_handle_frame\n");
	return 0;
}

static int switch_init(void) {
	printk("Hello, world!\n");
	sw_handle_frame_hook = sw_handle_frame;
	return 0;
}

static void switch_exit(void) {
	sw_handle_frame_hook = NULL;
}

module_init(switch_init);
module_exit(switch_exit);

#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Cool stuff");
MODULE_LICENSE("GPL");

static int switch_init(void) {
	printk("Hello, world!\n");
	return 0;
}

static void switch_exit(void) {
}

module_init(switch_init);
module_exit(switch_exit);

#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x520b1ddd, "struct_module" },
	{ 0x591b5fac, "swioctl_set" },
	{ 0x919e5558, "kmem_cache_destroy" },
	{ 0xe051a0ac, "__mod_timer" },
	{ 0x5168b821, "__kfree_skb" },
	{ 0xda264347, "del_timer" },
	{ 0xd23f294b, "sw_handle_frame_hook" },
	{ 0x31c8abea, "malloc_sizes" },
	{ 0xc2d88038, "rtnl_sem" },
	{ 0x1751f3de, "sub_preempt_count" },
	{ 0x74f7ea36, "skb_clone" },
	{ 0xb1eb21c3, "dev_get_by_name" },
	{ 0xb71be8ae, "remove_proc_entry" },
	{ 0xbd50ba4a, "alloc_netdev" },
	{ 0x8706fba7, "call_rcu" },
	{ 0x1d26aa98, "sprintf" },
	{ 0x245ec8e8, "strncpy_from_user" },
	{ 0x7d11c268, "jiffies" },
	{ 0xd533bec7, "__might_sleep" },
	{ 0x3a438b6d, "proc_mkdir" },
	{ 0xaf2fda33, "proc_net" },
	{ 0x1b7d4074, "printk" },
	{ 0xa092a2ae, "free_netdev" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0xc01bc056, "register_netdev" },
	{ 0x959eaa78, "netif_receive_skb" },
	{ 0xa3fd86c8, "kmem_cache_free" },
	{ 0x77aa845f, "dev_close" },
	{ 0xe5e806f9, "add_preempt_count" },
	{ 0x56662b32, "mod_timer" },
	{ 0x6091797f, "synchronize_rcu" },
	{ 0x707f93dd, "preempt_schedule" },
	{ 0x8dae653b, "dev_open" },
	{ 0xedabafd7, "skb_copy_expand" },
	{ 0x48ea5182, "kmem_cache_alloc" },
	{ 0x799aca4, "local_bh_enable" },
	{ 0x44de5676, "skb_under_panic" },
	{ 0x6eae70d0, "create_proc_entry" },
	{ 0x3a754765, "pskb_expand_head" },
	{ 0x925393c9, "ether_setup" },
	{ 0x25b2d2a6, "kmem_cache_create" },
	{ 0x463e37ad, "init_timer" },
	{ 0xb08b4248, "dev_set_promiscuity" },
	{ 0x37a0cba, "kfree" },
	{ 0x60a4461c, "__up_wakeup" },
	{ 0xadb6240d, "unregister_netdev" },
	{ 0x8235805b, "memmove" },
	{ 0x96b27088, "__down_failed" },
	{ 0x6a0428d1, "dev_queue_xmit" },
	{ 0xf2a644fb, "copy_from_user" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "B6E6A740101798A36457057");

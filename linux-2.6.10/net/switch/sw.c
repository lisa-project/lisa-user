/*
 *    This file is part of Linux Multilayer Switch.
 *
 *    Linux Multilayer Switch is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Linux Multilayer Switch is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Linux Multilayer Switch; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

struct net_switch sw;
/* Safely add an interface to the switch
 */

void dump_packet(const struct sk_buff *skb) {
	int i;
	
	printk(KERN_DEBUG "dev=%s: proto=0x%hx mac_len=%d "
			"head=0x%p data=0x%p tail=0x%p end=0x%p mac=0x%p\n",
			skb->dev->name, ntohs(skb->protocol), skb->mac_len,
			skb->head, skb->data, skb->tail, skb->end, skb->mac.raw);
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

	if(port->flags & SW_PFL_DISABLED) {
		dbg("Received frame on disabled port %s\n", port->dev->name);
		return 1;
	}

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

	return sw_forward(&sw, port, skb, &skb_e);
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

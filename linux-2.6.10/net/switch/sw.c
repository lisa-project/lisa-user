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
	if(skb->mac.raw)
		for(i = 0; i < skb->mac_len; i++)
			printk("0x%x ", skb->mac.raw[i]);
	printk("\nDATA dump: ");
	if(skb->data)
		for(i = 0; i < 4; i++)
			printk("0x%x ", skb->data[i]);
	printk("\n");
}

/* Handle a frame received on a physical interface */
__dbg_static int sw_handle_frame(struct net_switch_port *port, struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;
	struct skb_extra skb_e;

	if(port->flags & (SW_PFL_DISABLED | SW_PFL_DROPALL)) {
		dbg("Received frame on disabled port %s\n", port->dev->name);
		goto free_skb;
	}

	if(skb->protocol == ntohs(ETH_P_8021Q)) {
		skb_e.vlan = ntohs(*(unsigned short *)skb->data) & 4095;
		skb_e.has_vlan_tag = 1;
	} else {
		skb_e.vlan = port->vlan;
		skb_e.has_vlan_tag = 0;
	}

	/* Perform some sanity checks */
	if (!sw.vdb[skb_e.vlan]) {
		dbg("Vlan %d doesn't exist int the vdb\n", skb_e.vlan);
		goto free_skb;
	}
	if((port->flags & SW_PFL_TRUNK) && !skb_e.has_vlan_tag) {
		dbg("Received untagged frame on TRUNK port %s\n", port->dev->name);
		goto free_skb;
	}
	if(!(port->flags & SW_PFL_TRUNK) && skb_e.has_vlan_tag) {
		dbg("Received tagged frame on non-TRUNK port %s\n", port->dev->name);
		goto free_skb;
	}

	/* If interface is in trunk, check if the vlan is allowed */
	if((port->flags & SW_PFL_TRUNK) &&
			(port->forbidden_vlans[skb_e.vlan / 8] & (1 << (skb_e.vlan % 8)))) {
		dbg("Received frame on vlan %d, which is forbidden on %s\n",
				skb_e.vlan, port->dev->name);
		goto free_skb;
	}

	/* Update the fdb */
	fdb_learn(skb->mac.raw + 6, port, skb_e.vlan, SW_FDB_DYN);

	sw_forward(port, skb, &skb_e);
	return NET_RX_DROP;

free_skb:
	dev_kfree_skb(skb);
	return NET_RX_DROP;
}

/* Enable a port */
void sw_enable_port(struct net_switch_port *port) {
	/* Someday this will trigger some user-space callback to help
	   the cli display warnings about a port changing state. For
	   now just set the disabled flag */
	if(!(port->flags & SW_PFL_DISABLED) || port->flags & SW_PFL_ADMDISABLED)
		return;
	port->flags &= ~SW_PFL_DISABLED;
	dbg("Enabled port %s\n", port->dev->name);
}

/* Disable a port */
void sw_disable_port(struct net_switch_port *port) {
	/* Someday this will trigger some user-space callback to help
	   the cli display warnings about a port changing state. For
	   now just set the disabled flag */
	if(port->flags & SW_PFL_DISABLED)
		return;
	port->flags |= SW_PFL_DISABLED;
	dbg("Disabled port %s\n", port->dev->name);
}

/* Initialize everything associated with a switch */
static void init_switch(struct net_switch *sw) {
	int i;
	
	INIT_LIST_HEAD(&sw->ports);
	for (i=0; i<SW_VIF_HASH_SIZE; i++) {
		INIT_LIST_HEAD(&sw->vif[i]);
	}
	memcpy(sw->vif_mac, "\0lms\0\0", 6);
	/* TODO module parameter to initialize vif_mac */
	atomic_set(&sw->fdb_age_time, SW_DEFAULT_AGE_TIME); 
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
	sw_vif_cleanup(sw);
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

#ifdef DEBUG
EXPORT_SYMBOL(sw_handle_frame);
#endif

module_init(switch_init);
module_exit(switch_exit);

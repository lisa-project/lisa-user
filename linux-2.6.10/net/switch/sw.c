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

/* Handle a frame received on a physical interface

   1. This is the general picture of the packet flow:
   		driver_poll() {
			dev_alloc_skb();
			eth_copy_and_sum();
			eth_type_trans();
			netif_receive_skb() {
				skb->dev->poll && netpoll_rx(skb);
				deliver_skb();
				(struct packet_type).func(skb, ...); // general packet handlers
				handle_switch() {
					sw_handle_frame(skb);
				}
			}
		}
	  
	  driver_poll() is either the poll method of the driver (if it uses
	  NAPI) or the poll method of the backlog device, (if the driver
	  uses old Softnet).

   2. This function and all called functions rely on rcu being already
      locked. This is normally done in netif_receive_skb(). If for any
	  stupid reason this doesn't happen, things will go terribly wrong.

   3. Our return value is propagated back to driver_poll(). We should
      return NET_RX_DROP if the packet was discarded for any reason or
	  NET_RX_SUCCES if we handled the packet. We should'n however return
	  NET_RX_DROP if we touched the packet in any way.
 */
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
	return NET_RX_SUCCESS;

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

#include <asm/system.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/config.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <net/snmp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/raw.h>
#include <net/checksum.h>
#include <linux/netfilter_ipv4.h>
#include <net/xfrm.h>
#include <linux/mroute.h>
#include <linux/netlink.h>

int sw_packet_handler(struct sk_buff *skb, struct net_device *dev,
		struct packet_type *pt) {
	struct iphdr *iph;

	//dbg("sw_packet_handler: we've got our packet handler :) \n");
	if (!strcmp(skb->dev->name, "eth0")) {
		dev_kfree_skb(skb);
		return 0;
	}
	dump_packet(skb);
	/* When the interface is in promisc. mode, drop all the crap
	 * that it receives, do not try to analyse it.
	 */
	if (skb->pkt_type == PACKET_OTHERHOST) {
		dbg("drop1\n");
		goto drop;
	}

	IP_INC_STATS_BH(IPSTATS_MIB_INRECEIVES);

	if ((skb = skb_share_check(skb, GFP_ATOMIC)) == NULL) {
		IP_INC_STATS_BH(IPSTATS_MIB_INDISCARDS);
		dbg("drop2\n");
		goto out;
	}

	if (!pskb_may_pull(skb, sizeof(struct iphdr))) {
		dbg("drop3\n");
		goto inhdr_error;
	}

	iph = skb->nh.iph;

	/*
	 *	RFC1122: 3.1.2.2 MUST silently discard any IP frame that fails the checksum.
	 *
	 *	Is the datagram acceptable?
	 *
	 *	1.	Length at least the size of an ip header
	 *	2.	Version of 4
	 *	3.	Checksums correctly. [Speed optimisation for later, skip loopback checksums]
	 *	4.	Doesn't have a bogus length
	 */

	if (iph->ihl < 5 || iph->version != 4) {
		dbg("drop4\n");
		goto inhdr_error;
	}

	if (!pskb_may_pull(skb, iph->ihl*4)) {
		dbg("drop5\n");
		goto inhdr_error;
	}

	iph = skb->nh.iph;

	if (ip_fast_csum((u8 *)iph, iph->ihl) != 0) {
		dbg("drop6\n");
		goto inhdr_error;
	}

	{
		__u32 len = ntohs(iph->tot_len); 
		if (skb->len < len || len < (iph->ihl<<2)) {
		dbg("drop7\n");
			goto inhdr_error;
		}

		/* Our transport medium may have padded the buffer out. Now we know it
		 * is IP we can trim to the true length of the frame.
		 * Note this now means skb->len holds ntohs(iph->tot_len).
		 */
		if (skb->len > len) {
			__pskb_trim(skb, len);
			if (skb->ip_summed == CHECKSUM_HW)
				skb->ip_summed = CHECKSUM_NONE;
		}
	}

	dbg("final drop\n");
	goto drop;

inhdr_error:
	IP_INC_STATS_BH(IPSTATS_MIB_INHDRERRORS);
drop:
        kfree_skb(skb);
out:
        return NET_RX_DROP;
}

struct packet_type  *sw_pt;

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
	dbg("Registering our packet handler\n");
	sw_pt = kmalloc(sizeof(struct packet_type), GFP_ATOMIC);
	memset(sw_pt, 0, sizeof(struct packet_type));
	sw_pt->type = htons(ETH_P_IP);
	sw_pt->func = sw_packet_handler;
	dev_add_pack(sw_pt);
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
	dev_remove_pack(sw_pt);
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
EXPORT_SYMBOL(sw_packet_handler);
#endif

module_init(switch_init);
module_exit(switch_exit);

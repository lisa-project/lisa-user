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

#include "sw_private.h"
#include "sw_debug.h"

static inline void add_vlan_tag(struct sk_buff *skb, int vlan) {
	memmove(skb->mac.raw-VLAN_TAG_BYTES, skb->mac.raw, 2 * ETH_ALEN);	
	skb->mac.raw -= VLAN_TAG_BYTES;
	skb_push(skb, VLAN_TAG_BYTES);
	*(short *)skb->data = htons((short)vlan);
	*(short *)(skb->mac.raw + ETH_HLEN - 2) = htons(ETH_P_8021Q);
}

static inline void strip_vlan_tag(struct sk_buff *skb) {
	memmove(skb->mac.raw+VLAN_TAG_BYTES, skb->mac.raw, 2 * ETH_ALEN);
	skb->mac.raw+=VLAN_TAG_BYTES;
	skb_pull(skb, VLAN_TAG_BYTES);
	skb->protocol = *(short *)(skb->mac.raw + ETH_HLEN - 2);
}

/* Forward frame from in port to out port,
   adding/removing vlan tag if necessary.
*/
static void __sw_forward(struct net_switch_port *in, struct net_switch_port *out, 
	struct sk_buff *skb, struct skb_extra *skb_e) {

	if (out->flags & SW_PFL_TRUNK && !(in->flags & SW_PFL_TRUNK)) {
		/* must add vlan tag (vlan = in->vlan) */
		add_vlan_tag(skb, in->vlan);
	}
	else if (!(out->flags & SW_PFL_TRUNK) && in->flags & SW_PFL_TRUNK) {
		/* must remove vlan tag */
		strip_vlan_tag(skb);
	}
	dbg("Forwarding frame to %s\n", out->dev->name);
	skb->dev = out->dev;
	skb_push(skb, ETH_HLEN);
	dev_queue_xmit(skb);
}

/* Flood frame to all necessary ports */
static void sw_flood(struct net_switch *sw, struct net_switch_port *in,
		struct sk_buff *skb, int vlan) {
	struct net_switch_vdb_link *link;
	struct sk_buff *skb2, *skb3;
	
	/* if source port is in trunk mode we first send the 
	   socket buffer to all trunk ports in that vlan then
	   strip vlan tag and send to all non-trunk ports in that vlan */
	if (in->flags & SW_PFL_TRUNK) {
		list_for_each_entry_rcu(link, &sw->vdb[vlan]->trunk_ports, lh) {
			if (link->port == in) continue;
			skb2 = skb_clone(skb, GFP_ATOMIC);
			skb2->dev = link->port->dev;
			skb_push(skb2, ETH_HLEN);
			dev_queue_xmit(skb2);
		}
		skb2 = skb_copy(skb, GFP_ATOMIC);
		strip_vlan_tag(skb2);
		list_for_each_entry_rcu(link, &sw->vdb[vlan]->non_trunk_ports, lh) {
			if (link->port == in) continue;
			skb3 = skb_clone(skb2, GFP_ATOMIC);
			skb3->dev = link->port->dev;
			skb_push(skb3, ETH_HLEN);
			dev_queue_xmit(skb3);
		}
	} else { 
	/* otherwise we send the frame to all non-trunk ports in that vlan 
	   then add a vlan tag to it and send it to all trunk ports in that vlan.
	 */
		list_for_each_entry_rcu(link, &sw->vdb[vlan]->non_trunk_ports, lh) {
			if (link->port == in) continue;
			dbg("forward(flood): Forwarding frame to port %s\n", link->port->dev->name);
			skb2 = skb_clone(skb, GFP_ATOMIC);
			skb2->dev = link->port->dev;
			dbg("flood: before dev_queue_xmit()\n");
			skb_push(skb2, ETH_HLEN);
			dev_queue_xmit(skb2);
		}
		dump_mem(skb, sizeof(struct sk_buff));
		skb2 = skb_copy(skb, GFP_ATOMIC);
		dump_mem(skb2, sizeof(struct sk_buff));
		add_vlan_tag(skb2, vlan);
		list_for_each_entry_rcu(link, &sw->vdb[vlan]->trunk_ports, lh) {
			if (link->port == in) continue;
			skb3 = skb_clone(skb2, GFP_ATOMIC);
			skb3->dev = link->port->dev;
			skb_push(skb3, ETH_HLEN);
			dev_queue_xmit(skb3);
		}
	}
}

/* Forwarding decision */
int sw_forward(struct net_switch *sw, struct net_switch_port *in,
		struct sk_buff *skb, struct skb_extra *skb_e) {
	struct net_switch_bucket *bucket = &sw->fdb[sw_mac_hash(skb->mac.raw)];
	struct net_switch_fdb_entry *out;
	
	read_lock(&bucket->lock);
	if (fdb_lookup(bucket, skb->mac.raw, skb_e->vlan, &out)) {
		read_unlock(&bucket->lock);
		/* fdb entry found */
		if (in == out->port) {
			/* in_port == out_port */
			dbg("forward: Dropping frame, dport %s == sport %s\n",
				out->port->dev->name, in->dev->name);
			return 1; 
		}
		if (!(out->port->flags & SW_PFL_TRUNK) && 
				skb_e->vlan != out->port->vlan) {
			dbg("forward: Dropping frame, dport %s vlan_id %d != skb_e.vlan_id %d\n",
				out->port->dev->name, out->port->vlan, skb_e->vlan);
			return 1;
		}
		if ((out->port->flags & SW_PFL_TRUNK) &&
			(out->port->forbidden_vlans[skb_e->vlan / 8] & (1 << (skb_e->vlan % 8)))) {
			dbg("forward: Dropping frame, skb_e.vlan_id %d not in allowed vlans of dport %s\n",
				skb_e->vlan, out->port->dev->name);
			return 1;
		}
		dbg("forward: Forwarding frame from %s to %s\n", in->dev->name,
				out->port->dev->name);
		__sw_forward(in, out->port, skb, skb_e);
	} else {
		read_unlock(&bucket->lock);
		dbg("forward: Flooding frame from %s to all necessary ports\n",
				in->dev->name);
		sw_flood(sw, in, skb, skb_e->vlan);
	}	
	
	return 1;
}

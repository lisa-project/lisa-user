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

__dbg_static inline void add_vlan_tag(struct sk_buff **skb, int vlan) {
	struct sk_buff *skb2 = *skb;

	/* If we don't have enough headroom, we make some */
	if (skb2->data - skb2->head < ETH_HLEN + VLAN_TAG_BYTES) {
		skb2 = skb_copy_expand(*skb, VLAN_TAG_BYTES, 0, GFP_ATOMIC); 
		dbg("add_vlan_tag: skb_copy_expand necessary\n");
		dbg("add_vlan_tag(after expand): skb=0x%p skb headroom: %d (head=0x%p data=0x%p)\n",
			skb2, skb2->data - skb2->head, skb2->head, skb2->data);
		dev_kfree_skb(*skb);
		*skb = skb2;
	}
	memmove(skb2->mac.raw-VLAN_TAG_BYTES, skb2->mac.raw, 2 * ETH_ALEN);	
	skb2->mac.raw -= VLAN_TAG_BYTES;
	skb_push(skb2, VLAN_TAG_BYTES);
	*(short *)skb2->data = htons((short)vlan);
	*(short *)(skb2->mac.raw + ETH_HLEN - 2) = htons(ETH_P_8021Q);
}

__dbg_static inline void strip_vlan_tag(struct sk_buff *skb) {
	memmove(skb->mac.raw+VLAN_TAG_BYTES, skb->mac.raw, 2 * ETH_ALEN);
	skb->mac.raw+=VLAN_TAG_BYTES;
	skb_pull(skb, VLAN_TAG_BYTES);
	skb->protocol = *(short *)(skb->mac.raw + ETH_HLEN - 2);
}

__dbg_static inline void __strip_vlan_tag(struct sk_buff **skb, int i) {
	strip_vlan_tag(*skb);
}

/* FIXME: pkt_type may be PACKET_MULTICAST */
__dbg_static inline void sw_skb_xmit(struct sk_buff *skb, struct net_device *dev,
		unsigned char pkt_type) {
	/* FIXME pachetele de 1500 se busesc
	if (skb->len > skb->dev->mtu) {
		dbg("Dropping frame due to len > dev->mtu\n");
		goto destroy;
	}
	*/
	if (dev->sw_port) {
		/* This is a physical port (not a bogus one i.e. vif) */
		skb->dev = dev;
		skb_push(skb, ETH_HLEN);
		dev_queue_xmit(skb);
		return;
	}
	/* This is a vif, so we need to call sw_vif_rx() instead */
	skb->pkt_type = pkt_type;
	skb->dev = dev;
	sw_vif_rx(skb);
	return;
	
/* destroy:	*/
	dev_kfree_skb(skb);
}

/* if the packet data is used by someone else
   we make a copy before altering it 
 */
__dbg_static void sw_skb_unshare(struct sk_buff **skb) {
	struct sk_buff *skb2;

	if (atomic_read(&skb_shinfo(*skb)->dataref)) {
		skb2 = skb_copy_expand(*skb, VLAN_TAG_BYTES, 0, GFP_ATOMIC);
		dev_kfree_skb(*skb);
		*skb = skb2;
	}
}

/* Forward frame from in port to out port,
   adding/removing vlan tag if necessary.
 */
__dbg_static void __sw_forward(struct net_switch_port *in, struct net_switch_port *out, 
	struct sk_buff *skb, struct skb_extra *skb_e) {

	if (out->flags & SW_PFL_TRUNK && !(in->flags & SW_PFL_TRUNK)) {
		/* must add vlan tag (vlan = in->vlan) */
		sw_skb_unshare(&skb);
		add_vlan_tag(&skb, in->vlan);
	}
	else if (!(out->flags & SW_PFL_TRUNK) && in->flags & SW_PFL_TRUNK) {
		/* must remove vlan tag */
		sw_skb_unshare(&skb);
		strip_vlan_tag(skb);
	}
	dbg("Forwarding frame to %s\n", out->dev->name);
	sw_skb_xmit(skb, out->dev, PACKET_HOST);
}

#ifdef DEBUG
#define __sw_flood_inc_cloned cloned++
#define __sw_flood_inc_copied copied++
#define __sw_flood_inc_unshared unshared++
#else
#define __sw_flood_inc_cloned
#define __sw_flood_inc_copied
#define __sw_flood_inc_unshared
#endif
__dbg_static int __sw_flood(struct net_switch *sw, struct net_switch_port *in,
	struct sk_buff *skb, int vlan, void (*f)(struct sk_buff **, int),
	struct list_head *lh1, struct list_head *lh2) {
#ifdef DEBUG
	int cloned = 0, copied = 0, unshared = 0;
#endif
	
	struct net_switch_vdb_link *link, *prev=NULL, *oldprev;
	struct sk_buff *skb2;
	int needs_tag_change = 1;
	int ret = 0;

	list_for_each_entry_rcu(link, lh1, lh) {
		if (link->port == in) continue;
		if (prev) {
			__sw_flood_inc_cloned;
			skb2 = skb_clone(skb, GFP_ATOMIC);
			sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
			ret++;
			skb = skb2;
		}
		prev = link;
	}
	oldprev = prev;
	list_for_each_entry_rcu(link, lh2, lh) {
		if (link->port == in) continue;
		if (oldprev && prev == oldprev) {
			/* 1 or more elements in lh1 && and we're at the first element
			   in lh2; make a copy of the skb, then send the last skb from
			   lh1
			 */
			__sw_flood_inc_copied;
			skb2 = skb_copy_expand(skb, VLAN_TAG_BYTES, 0, GFP_ATOMIC);
			f(&skb2, vlan);
			needs_tag_change = 0;
			sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
			ret++;
			skb = skb2;
			prev = link;
			continue;
		}
		if (prev) {
			if (needs_tag_change) {
				/* 0 elements in lh1, and we're at the 2nd element in lh2;
				   make sure skb is an exclusive copy and apply the tag
				   change to it before it gets cloned and sent
				 */
				__sw_flood_inc_unshared;
				sw_skb_unshare(&skb);
				f(&skb, vlan);
				needs_tag_change = 0;
			}
			__sw_flood_inc_cloned;
			skb2 = skb_clone(skb, GFP_ATOMIC);

			sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
			ret++;
			skb = skb2;
		}
		prev = link;
	}
	if (prev) {
		if (needs_tag_change && prev != oldprev) {
			/* lh2 is not empty, so the remaining element is from lh2,
			   but the tag change was not applied
			 */
			__sw_flood_inc_unshared;
			sw_skb_unshare(&skb);
			f(&skb, vlan);
		}	
		sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
		ret++;
	}
	else {
		dbg("flood: nothing to flood, freeing skb.\n");
		dev_kfree_skb(skb);
	}
	dbg("__sw_flood: cloned=%d copied=%d unshared=%d\n", cloned, copied,
			unshared);
	return ret;
}

/* Walk a fdb bucket and send a packet to all ports that match a
   given mac address. This is used with multicast destination macs,
   which are never dynamically learnt.

   This function uses exactly the same "packet change optimization" (when
   sending to multiple ports, the packet is changed at most once i.e. a
   vlan tag is stripped or added) as __sw_flood above.

   For better performance, I prefered to copy the whole function/algorithm
   instead of making it more abstract. This means that changes to the algorithm
   should be made in parallel (both in __sw_flood and __sw_multicast). And if
   one of them is buggy, then most probably the other one is too :P
 */
__dbg_static int __sw_multicast(struct net_switch *sw, struct net_switch_port *in,
	struct sk_buff *skb, int vlan, void (*f)(struct sk_buff **, int),
	struct list_head *fdb_entry_lh, int first_trunkness) {
#ifdef DEBUG
	int cloned = 0, copied = 0, unshared = 0;
#endif
	
	struct net_switch_fdb_entry *entry, *prev=NULL, *oldprev;
	struct sk_buff *skb2;
	int needs_tag_change = 1;
	int ret = 0;

	list_for_each_entry_rcu(entry, fdb_entry_lh, lh) {
		/* First check trunkness, so that we don't compare macs both
		   now and the second time we walk the list
		 */
		if ((entry->port->flags & SW_PFL_TRUNK) != first_trunkness)
			continue;
		if (entry->vlan != vlan)
			continue;
		if (entry->port == in) continue;
		if (memcmp(entry->mac, skb->mac.raw, ETH_ALEN))
			continue;
		/* In __sw_flood we use the vdb, so we know for sure that 'vlan' is
		   allowed on the output port. But here we walk the fdb, so we need
		   to additionally check if 'vlan' is allowed on the output port.
		 */
		if ((first_trunkness && sw_port_forbidden_vlan(entry->port, vlan)) ||
				(!first_trunkness && entry->port->vlan != vlan))
			continue;
		if (prev) {
			__sw_flood_inc_cloned;
			skb2 = skb_clone(skb, GFP_ATOMIC);
			/* FIXME: PACKET_MULTICAST */
			sw_skb_xmit(skb, prev->port->dev, PACKET_MULTICAST);
			ret++;
			skb = skb2;
		}
		prev = entry;
	}
	oldprev = prev;
	list_for_each_entry_rcu(entry, fdb_entry_lh, lh) {
		if ((entry->port->flags & SW_PFL_TRUNK) == first_trunkness)
			continue;
		if (entry->vlan != vlan)
			continue;
		if (entry->port == in) continue;
		if (memcmp(entry->mac, skb->mac.raw, ETH_ALEN))
			continue;
		if ((first_trunkness && sw_port_forbidden_vlan(entry->port, vlan)) ||
				(!first_trunkness && entry->port->vlan != vlan))
			continue;
		if (oldprev && prev == oldprev) {
			/* 1 or more elements in lh1 && and we're at the first element
			   in lh2; make a copy of the skb, then send the last skb from
			   lh1
			 */
			__sw_flood_inc_copied;
			skb2 = skb_copy_expand(skb, VLAN_TAG_BYTES, 0, GFP_ATOMIC);
			f(&skb2, vlan);
			needs_tag_change = 0;
			sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
			ret++;
			skb = skb2;
			prev = entry;
			continue;
		}
		if (prev) {
			if (needs_tag_change) {
				/* 0 elements in lh1, and we're at the 2nd element in lh2;
				   make sure skb is an exclusive copy and apply the tag
				   change to it before it gets cloned and sent
				 */
				__sw_flood_inc_unshared;
				sw_skb_unshare(&skb);
				f(&skb, vlan);
				needs_tag_change = 0;
			}
			__sw_flood_inc_cloned;
			skb2 = skb_clone(skb, GFP_ATOMIC);

			sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
			ret++;
			skb = skb2;
		}
		prev = entry;
	}
	if (prev) {
		if (needs_tag_change && prev != oldprev) {
			/* lh2 is not empty, so the remaining element is from lh2,
			   but the tag change was not applied
			 */
			__sw_flood_inc_unshared;
			sw_skb_unshare(&skb);
			f(&skb, vlan);
		}	
		sw_skb_xmit(skb, prev->port->dev, PACKET_BROADCAST);
		ret++;
	}
	else {
		dbg("multicast: no ports, you don't want to free the skb!.\n");
		/* Don't free the skb this time like we do in sw_flood, because
		   the packet won't be dropped. Instead it will be flooded to all
		   ports later in sw_forward().
		 */
	}
	dbg("__sw_multicast: cloned=%d copied=%d unshared=%d\n", cloned, copied,
			unshared);
	return ret;
}
#undef __sw_flood_inc_cloned
#undef __sw_flood_inc_copied
#undef __sw_flood_inc_unshared

/* Flood frame to all necessary ports */
__dbg_static int sw_flood(struct net_switch *sw, struct net_switch_port *in,
		struct sk_buff *skb, int vlan) {

	/* if source port is in trunk mode we first send the 
	   socket buffer to all trunk ports in that vlan then
	   strip vlan tag and send to all non-trunk ports in that vlan 
	 */
	if (in->flags & SW_PFL_TRUNK) {
		return __sw_flood(sw, in, skb, vlan, __strip_vlan_tag,
				&sw->vdb[vlan]->trunk_ports,
				&sw->vdb[vlan]->non_trunk_ports);
	}
	else {
	/* otherwise we send the frame to all non-trunk ports in that vlan 
	   then add a vlan tag to it and send it to all trunk ports in that vlan.
	 */
		return __sw_flood(sw, in, skb, vlan, add_vlan_tag,
				&sw->vdb[vlan]->non_trunk_ports,
				&sw->vdb[vlan]->trunk_ports);
	}
}	

int sw_vif_forward(struct sk_buff *skb, struct skb_extra *skb_e) {
	struct net_switch *sw = (skb->dev->sw_port)? skb->dev->sw_port->sw:
		((struct net_switch_vif_priv *)netdev_priv(skb->dev))->bogo_port.sw;
	unsigned char *vif_mac = sw->vif_mac;
	struct net_device *dev;
	int vlan;

	if(memcmp(skb->mac.raw, vif_mac, ETH_ALEN - 2))
		return 0;
	vlan = (vif_mac[ETH_ALEN - 2] ^ skb->mac.raw[ETH_ALEN - 2]) * 0x100 +
		(vif_mac[ETH_ALEN - 1] ^ skb->mac.raw[ETH_ALEN - 1]);
	if(vlan == skb_e->vlan && (dev = sw_vif_find(sw, vlan))) {
		if(skb_e->has_vlan_tag) {
			sw_skb_unshare(&skb);
			strip_vlan_tag(skb);
		}
		skb->pkt_type = PACKET_HOST;
		skb->dev = dev;
		sw_vif_rx(skb);
		return 1;
	}
	return 0;
}

/* Forwarding decision
   Returns the number of ports the packet was forwarded to.
 */
int sw_forward(struct net_switch_port *in,
		struct sk_buff *skb, struct skb_extra *skb_e) {
	struct net_switch *sw = in->sw;
	struct net_switch_bucket *bucket = &sw->fdb[sw_mac_hash(skb->mac.raw)];
	struct net_switch_fdb_entry *out;
	int ret = 1;

	dbg("sw_forward: usage count %d\n", atomic_read(&skb_shinfo(skb)->dataref) != 1);
	if (sw_vif_forward(skb, skb_e))
		return ret;
	rcu_read_lock();
	if (is_mcast_mac(skb->mac.raw)) {
		ret = __sw_multicast(sw, in, skb, skb_e->vlan,
				in->flags & SW_PFL_TRUNK ? __strip_vlan_tag : add_vlan_tag,
				&bucket->entries, in->flags & SW_PFL_TRUNK);
		rcu_read_unlock();
		if(ret)
			return ret;
		ret = sw_flood(sw, in, skb, skb_e->vlan);
		return ret;
	}
	if (fdb_lookup(bucket, skb->mac.raw, skb_e->vlan, &out)) {
		/* fdb entry found */
		rcu_read_unlock();
		if (in == out->port) {
			/* in_port == out_port */
			dbg("forward: Dropping frame, dport %s == sport %s\n",
				out->port->dev->name, in->dev->name);
			goto free_skb; 
		}
		if (!(out->port->flags & SW_PFL_TRUNK) && 
				skb_e->vlan != out->port->vlan) {
			dbg("forward: Dropping frame, dport %s vlan_id %d != skb_e.vlan_id %d\n",
				out->port->dev->name, out->port->vlan, skb_e->vlan);
			goto free_skb;
		}
		if ((out->port->flags & SW_PFL_TRUNK) &&
			(out->port->forbidden_vlans[skb_e->vlan / 8] & (1 << (skb_e->vlan % 8)))) {
			dbg("forward: Dropping frame, skb_e.vlan_id %d not in allowed vlans of dport %s\n",
				skb_e->vlan, out->port->dev->name);
			goto free_skb;
		}
		dbg("forward: Forwarding frame from %s to %s\n", in->dev->name,
				out->port->dev->name);
		__sw_forward(in, out->port, skb, skb_e);
	} else {
		rcu_read_unlock();
		dbg("forward: Flooding frame from %s to all necessary ports\n",
				in->dev->name);
		/*
		   The fact that skb_e->vlan exists in the vdb is based
		   _only_ on the checks performed in sw_handle_frame()
		 */
		ret = sw_flood(sw, in, skb, skb_e->vlan);
	}	
	return ret; 
	
free_skb:	
	dev_kfree_skb(skb);
	return 0;
}

#include "sw_private.h"
#include <linux/netdevice.h>

#define JIFFIES_TO_TICKS(j) (((j) << 8) / HZ)
#define TICKS_TO_JIFFIES(j) (((j) * HZ) >> 8)

//const unsigned char stp_multicast_mac[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

static inline void set_ticks(unsigned char* dest, int jiff)
{
	__u16 ticks = JIFFIES_TO_TICKS(jiff);
	
	dest[0] = (ticks >> 8) & 0xFF;
	dest[1] = ticks & 0xFF;
}

static inline int get_ticks(unsigned char* src)
{
	return TICKS_TO_JIFFIES((*src << 8) | *(src+1));	
}


void send_bpdu(struct net_switch_port* p, unsigned char* buf, int length)
{
	struct sk_buff* skb;
	int size;
	 
	if(!atomic_read(&p->sw->stp_vars.stp_enabled))
		return;
	
	size = length + 2 * ETH_ALEN + 2;
	if(size < 60)
		size = 60;
	
	if((skb = dev_alloc_skb(size)) == NULL)
	{
		printk(KERN_INFO "STP sw_stp_bpdu.c  -> send_bpdu: No memory available to create skb.\n");
		return;
	}
	
	skb->dev = p->dev;
	skb->protocol = htons(ETH_P_802_2);
	skb->mac.raw = skb_put(skb, size);
	memcpy(skb->mac.raw, stp_multicast_mac, ETH_ALEN);	
	memcpy(skb->mac.raw + ETH_ALEN, p->dev->dev_addr, ETH_ALEN);
	skb->mac.raw[2*ETH_ALEN] = 0;
	skb->mac.raw[2*ETH_ALEN+1] = length;
	skb->nh.raw = skb->mac.raw + 2*ETH_ALEN + 2;
	memcpy(skb->nh.raw, buf, length);
	memset(skb->nh.raw + length, 0xa5, size - length - 2*ETH_ALEN - 2);
	
	//printk(KERN_INFO "Dev %s: send bpdu\n", p->dev->name);
	if(dev_queue_xmit(skb) < 0)
		printk(KERN_INFO "Dev %s: could not send bpdu\n", p->dev->name);
}

void send_config_bpdu(struct net_switch_port* p, struct sw_stp_bpdu* bpdu)
{
	unsigned char buf[38];
	
	buf[0] = 0x42;
	buf[1] = 0x42;
	buf[2] = 0x03;
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = BPDU_TYPE_CONFIG;
	buf[7] = bpdu->flags;
	buf[8] = bpdu->root_id.prio[0];
	buf[9] = bpdu->root_id.prio[1];
	buf[10] = bpdu->root_id.addr[0];
	buf[11] = bpdu->root_id.addr[1];
	buf[12] = bpdu->root_id.addr[2];
	buf[13] = bpdu->root_id.addr[3];
	buf[14] = bpdu->root_id.addr[4];
	buf[15] = bpdu->root_id.addr[5];
	buf[16] = (bpdu->root_path_cost >> 24) & 0xFF;
	buf[17] = (bpdu->root_path_cost >> 16) & 0xFF;
	buf[18] = (bpdu->root_path_cost >> 8) & 0xFF;
	buf[19] = bpdu->root_path_cost & 0xFF;
	buf[20] = bpdu->bridge_id.prio[0];
	buf[21] = bpdu->bridge_id.prio[1];
	buf[22] = bpdu->bridge_id.addr[0];
	buf[23] = bpdu->bridge_id.addr[1];
	buf[24] = bpdu->bridge_id.addr[2];
	buf[25] = bpdu->bridge_id.addr[3];
	buf[26] = bpdu->bridge_id.addr[4];
	buf[27] = bpdu->bridge_id.addr[5];
	buf[28] = (bpdu->port_id >> 8) & 0xFF;
	buf[29] = bpdu->port_id & 0xFF;
	
	set_ticks(buf+30, bpdu->message_age);
	set_ticks(buf+32, bpdu->max_age);
	set_ticks(buf+34, bpdu->hello_time);
	set_ticks(buf+36, bpdu->forward_delay);
	
	send_bpdu(p, buf, 38);
	
	//printk(KERN_INFO "Dev name send config : %s\n", p->dev->name);
}



static unsigned const char stp_header[6] = {0x42, 0x42, 0x03, 0x00, 0x00, 0x00};


/* Transmit a topology changed notification */
void send_tcn_bpdu(struct net_switch_port* port)
{
  unsigned char buf[7];

  buf[0] = stp_header[0];
  buf[1] = stp_header[1];
  buf[2] = stp_header[2];
  buf[3] = stp_header[3];
  buf[4] = stp_header[4];
  buf[5] = stp_header[5];
  buf[6] = BPDU_TYPE_TCN;

  send_bpdu(port, buf, 7);
}

/* Handles a received BPDU and determines its type and calling the apropriate functions. */
void sw_stp_handle_bpdu(struct sk_buff* skb)
{
	unsigned char* buf;
	struct net_switch_port* p = rcu_dereference(skb->dev->sw_port);
	
	//printk(KERN_INFO "stp_handle_bpdu: received bpdu \n");
	
	if(!p)
		goto err;
	
	spin_lock(&p->sw->stp_vars.lock);
	
	/* Check if port is disabled*/
	if(is_disabled_port(p))
		goto out;
	
	/* Check if STP is enabled per switch. */
	if(!atomic_read(&p->sw->stp_vars.stp_enabled))
		goto out;
	
	/* Check if frame contains the LLC STP header. */
	if(!pskb_may_pull(skb,sizeof(stp_header)) ||
	   memcmp(skb->data, stp_header, sizeof(stp_header)))
		goto out;
	
	/* Remove form skb->data the LLC STP header. */
	buf = skb_pull(skb, sizeof(stp_header));
	
	/* Check the type of the bpdu */
	if(buf[0] == BPDU_TYPE_CONFIG)
	{
		struct sw_stp_bpdu bpdu;
		
		/* Check if the remaining data from the config bpdu rcvd has the correct size. */
		if(!pskb_may_pull(skb, 32))
			goto out;
		
		/* Construct the BPDU from the skb->data. */
		bpdu.flags = buf[1];
		bpdu.root_id.prio[0] = buf[2];
		bpdu.root_id.prio[1] = buf[3];
		bpdu.root_id.addr[0] = buf[4];
		bpdu.root_id.addr[1] = buf[5];
		bpdu.root_id.addr[2] = buf[6];
		bpdu.root_id.addr[3] = buf[7];
		bpdu.root_id.addr[4] = buf[8];
		bpdu.root_id.addr[5] = buf[9];
		bpdu.root_path_cost =
			(buf[10] << 24) |
			(buf[11] << 16) |
			(buf[12] << 8) |
			buf[13];
		
		bpdu.bridge_id.prio[0] = buf[14];
		bpdu.bridge_id.prio[1] = buf[15];
		bpdu.bridge_id.addr[0] = buf[16];
		bpdu.bridge_id.addr[1] = buf[17];
		bpdu.bridge_id.addr[2] = buf[18];
		bpdu.bridge_id.addr[3] = buf[19];
		bpdu.bridge_id.addr[4] = buf[20];
		bpdu.bridge_id.addr[5] = buf[21];
		bpdu.port_id = (buf[22] << 8) | buf[23];
		
		bpdu.message_age = get_ticks(buf+24);
		bpdu.max_age = get_ticks(buf+26);
		bpdu.hello_time = get_ticks(buf+28);
		bpdu.forward_delay = get_ticks(buf+30);
		
		received_config_bpdu(p, &bpdu);
	}
	else if(buf[0] == BPDU_TYPE_TCN)
	{
		received_tcn_bpdu(p);
	}
	
out:
	spin_unlock(&p->sw->stp_vars.lock);
err:
	dev_kfree_skb(skb);
}



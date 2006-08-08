#include "sw_private.h"

void vtp_send_message(struct net_switch_port* p, unsigned char* buf, int length)
{
	struct sk_buff* skb;
	int size;
	 
	if(!atomic_read(&p->sw->vtp_vars.vtp_enabled))
		return;
	
	size = length + 2 * ETH_ALEN + 2 + VTP_LLC_HEADER_LEN + VTP_SNAP_HEADER_LEN;
	if(size < 60)
		size = 60;
	
	//printk(KERN_INFO "skb size = %d\n", size);
	
	if((skb = dev_alloc_skb(size)) == NULL)
	{
		printk(KERN_INFO "VTP vtp_send_message: No memory available to create skb.\n");
		return;
	}
	
	skb->dev = p->dev;
	skb->protocol = htons(ETH_P_802_2);
	skb->mac.raw = skb_put(skb, size);
	
	//set destination and source MAC
	memcpy(skb->mac.raw, vtp_multicast_mac, ETH_ALEN);	
	memcpy(skb->mac.raw + ETH_ALEN, p->dev->dev_addr, ETH_ALEN);
	
	//set frame size
	skb->mac.raw[2*ETH_ALEN] = ((length + VTP_LLC_HEADER_LEN + VTP_SNAP_HEADER_LEN) >> 8) & 0xFF;
	skb->mac.raw[2*ETH_ALEN+1] = (length + VTP_LLC_HEADER_LEN + VTP_SNAP_HEADER_LEN) & 0xFF;

	skb->nh.raw = skb->mac.raw + 2*ETH_ALEN + 2;
	
	//set llc header
	memcpy(skb->nh.raw, vtp_llc_header, VTP_LLC_HEADER_LEN);
	//set snap header
	memcpy(skb->nh.raw + VTP_LLC_HEADER_LEN, vtp_snap_header, VTP_SNAP_HEADER_LEN);

	//write the vtp message
	memcpy(skb->nh.raw + VTP_LLC_HEADER_LEN + VTP_SNAP_HEADER_LEN, buf, length);
	memset(skb->nh.raw + length, 0xa5, size - length - 2*ETH_ALEN - 2 - VTP_LLC_HEADER_LEN - VTP_SNAP_HEADER_LEN);

	/* Add 802.1q tag */
	add_vlan_tag(skb, 1);

	if(dev_queue_xmit(skb) < 0)
	  printk(KERN_INFO "Dev %s: could not send vtp message\n", p->dev->name);
}

int determine_vlan_type(int vlan)
{
	switch(vlan)
	{
		case VLAN_TYPE_FDDI_NO:
			return VLAN_TYPE_FDDI;
		case VLAN_TYPE_TRCRF_NO:
			return VLAN_TYPE_TRCRF;
		case VLAN_TYPE_FDDI_NET_NO:
			return VLAN_TYPE_FDDI_NET;
		case VLAN_TYPE_TRBRF_NO:
			return VLAN_TYPE_TRBRF;
	}
	
	return VLAN_TYPE_ETHERNET;
}

int determine_vlan_mtu(int vlan)
{
	switch(vlan)
	{
		case VLAN_TYPE_FDDI_NO:
			return FDDI_MTU;
		case VLAN_TYPE_TRCRF_NO:
			return TRCRF_MTU;
		case VLAN_TYPE_FDDI_NET_NO:
			return FDDI_NET_MTU;
		case VLAN_TYPE_TRBRF_NO:
			return TRBRF_MTU;
	}
	
	return ETHERNET_MTU;
}

struct vlan_info* create_vlan_info(struct net_switch* sw, int vlan)
{
	struct net_switch_vdb_entry* ventry = sw->vdb[vlan];
	struct vlan_info* vinfo;
	
	vinfo = (struct vlan_info*) kmalloc(sizeof(struct vlan_info), GFP_ATOMIC);
	if(!vinfo) 
		return NULL;
	
	vinfo->status = ventry->status;
	vinfo->type = determine_vlan_type(vlan);
	vinfo->name_len = strlen(ventry->name);
	vinfo->id = vlan;
	vinfo->mtu = determine_vlan_mtu(vlan);
	vinfo->dot10 = BASE_802DOT10_INDEX + vlan;
	memset(vinfo->name, 0, SW_MAX_VLAN_NAME+1);
	memcpy(vinfo->name, ventry->name, strlen(ventry->name));
	
	vinfo->len = 12 + vinfo->name_len + ((vinfo->name_len % 4) ? 4 - (vinfo->name_len % 4):0);
	
	INIT_LIST_HEAD(&vinfo->lh);
	
	return vinfo;
}

int add_subset_to_list(struct net_switch* sw, struct vtp_subset** vtp_sub, u8* seq)
{
	struct vtp_subset* sub;
		
	sub = (struct vtp_subset*) kmalloc (sizeof(struct vtp_subset), GFP_ATOMIC);
	if(sub == NULL)
		return -ENOMEM;
	
	sub->version = sw->vtp_vars.version;
	sub->code = VTP_SUBSET_ADVERT;
	sub->seq = ++(*seq);
	sub->dom_len = sw->vtp_vars.dom_len;
	memset(sub->domain, 0, VTP_DOMAIN_SIZE);
	memcpy(sub->domain, sw->vtp_vars.domain, sub->dom_len);
	sub->revision = sw->vtp_vars.revision;
	
	INIT_LIST_HEAD(&sub->infos);
	INIT_LIST_HEAD(&sub->subsets);
	
	list_add(&sub->subsets, &sw->vtp_vars.subsets);
	*vtp_sub = sub;
	
	return sizeof(*sub) - sizeof(sub->infos) - sizeof(sub->subsets) - sizeof(int);
}	


u8 create_subset_list(struct net_switch* sw)
{
	int vlan;
	struct vtp_subset* sub = NULL;
	struct vlan_info* info = NULL;
	int subset_size = 0;
	u8 subset_no = 0;
	int size = 0;
	
	for(vlan = SW_MIN_VLAN; vlan <= SW_MAX_VLAN; vlan++) 
	{
		rcu_read_lock();
		if(sw->vdb[vlan] == NULL) 
		{
			rcu_read_unlock();
			continue;
		}
		printk(KERN_INFO "%s\n", sw->vdb[vlan]->name);
				
		if(subset_size == 0)
		{
			//printk(KERN_INFO "New subset[%d]\n", subset_no);
			size = add_subset_to_list(sw, &sub, &subset_no);
			
			if(size < 1) 
			{
				rcu_read_unlock();
				continue;
			}
			
			subset_size += size;
			
			printk(KERN_INFO "New subset[%d] [%d]\n", subset_no, subset_size);
		}
		
		info = create_vlan_info(sw, vlan);
		if(!info) 
		{
			rcu_read_unlock();
			//printk(KERN_INFO "create_subset_list: info == NULL\n");
			continue;
		}
		
		if(subset_size + size > ETHERNET_MTU - VTP_LLC_HEADER_LEN - VTP_SNAP_HEADER_LEN)
		{
			//sub->length = subset_size;
			
			//printk(KERN_INFO "create_subset_list: sub[%d]->length = %d\n", sub->seq, sub->length);
			
			if(subset_no == 255)
			{
				rcu_read_unlock();
				return 255;
			}
			
			size = add_subset_to_list(sw, &sub, &subset_no);
			if(size < 1)
			{
				rcu_read_unlock();
				return 0;
			}
			
			subset_size = size;
			
			printk(KERN_INFO "New subset[%d] [%d]\n", subset_no, subset_size);
		}	
		
		list_add(&info->lh, &sub->infos);
		subset_size += info->len;
		sub->length = subset_size;
		
		printk(KERN_INFO "create_subset_list: subset_size = %d\n", subset_size);
				
		rcu_read_unlock();
	}
	
	return subset_no;
}

void destroy_subset(struct vtp_subset* sub)
{
	struct vlan_info* info;
	struct list_head *i, *l;
	
	list_for_each_safe(i, l,  &sub->infos)
	{
		info = list_entry(i, struct vlan_info, lh);
			
		list_del(&info->lh);
		kfree(info);
	}

	kfree(sub);
}

void destroy_subset_list(struct net_switch* sw)
{
	struct vtp_subset* sub;
	struct list_head *s, *n;
	
	list_for_each_safe(s, n, &sw->vtp_vars.subsets)
	{
		sub = list_entry(s, struct vtp_subset, subsets);
		list_del(&sub->subsets);
		destroy_subset(sub);
	}
}

//static inline 
u8 determine_followers(struct net_switch* sw, u16 start)
{
	u8 size = 0;

	printk(KERN_INFO "determine_followers: subset_list_size = %d --- start = %d\n", 
		sw->vtp_vars.subset_list_size, start);
	
	if(sw->vtp_vars.subset_list_size)
	{
		return sw->vtp_vars.subset_list_size - start;
	}
	
	size = create_subset_list(sw);
	
	printk(KERN_INFO "determine_followers: subset list size %d\n", size);
	
	if(size < 1)
	{
		destroy_subset_list(sw);
		sw->vtp_vars.subset_list_size = 0;
		return 0;
	}
	
	sw->vtp_vars.subset_list_size = size;
	
	return size;
}


static inline int create_summary(struct net_switch* sw, unsigned char* buf, u8 ctxt, u16 start)
{
	buf[0] = sw->vtp_vars.version;
	buf[1] = VTP_SUMM_ADVERT;
	
	if(ctxt == PROCESS_CONTEXT)
		buf[2] = determine_followers(sw, start);
	else
		buf[2] = 0;
	
	printk(KERN_INFO "create_summary: followers = %d\n", buf[2]);
	
	buf[3] = sw->vtp_vars.dom_len;
	
	memcpy(buf+4, &sw->vtp_vars.domain, VTP_DOMAIN_SIZE);
	
	buf[36] = (sw->vtp_vars.revision >> 24) & 0XFF;
	buf[37] = (sw->vtp_vars.revision >> 16) & 0XFF;
	buf[38] = (sw->vtp_vars.revision >> 8) & 0XFF;
	buf[39] = sw->vtp_vars.revision & 0XFF;
	
	buf[40] = sw->vtp_vars.last_updater[0];
	buf[41] = sw->vtp_vars.last_updater[1];
	buf[42] = sw->vtp_vars.last_updater[2];
	buf[43] = sw->vtp_vars.last_updater[3];
	
	memcpy(buf+44, &sw->vtp_vars.last_update, VTP_TIMESTAMP_SIZE);
	memcpy(buf+56, &sw->vtp_vars.md5, VTP_MD5_SIZE);
	
	return sizeof(struct vtp_summary);
}

void vtp_send_summary(struct net_switch* sw, u8 ctxt, u16 start)
{
	struct net_switch_port* p;
	unsigned char buf[sizeof(struct vtp_summary)];
	int len;
	
	memset(buf, 0, sizeof(struct vtp_summary));
	len = create_summary(sw, buf, ctxt, start);
	
	//printk(KERN_INFO "vtp_send_summary: followers = %d\n", buf[2]);
	
	list_for_each_entry(p, &sw->ports, lh) 
	{
		if(p->flags & SW_PFL_TRUNK)
			vtp_send_message(p, buf, len);
	}
}

static inline void create_subset(struct vtp_subset* sub, unsigned char* buf)
{
	struct list_head* i;
	struct vlan_info* info;
	int pos = 39;
	
	//printk(KERN_INFO "Subset[%d]:\n", sub->seq);
	
	buf[0] = sub->version;
	buf[1] = VTP_SUBSET_ADVERT;
	buf[2] = sub->seq;
	buf[3] = sub->dom_len;
	memcpy(buf+4, sub->domain, VTP_DOMAIN_SIZE);
	buf[36] = (sub->revision >> 24) & 0xFF;
	buf[37] = (sub->revision >> 16) & 0xFF;
	buf[38] = (sub->revision >> 8) & 0xFF;
	buf[39] = sub->revision & 0xFF;
	
	list_for_each(i, &sub->infos)
	{
		info = list_entry(i, struct vlan_info, lh);

		buf[++pos] = info->len; 
		buf[++pos] = info->status;
		buf[++pos] = info->type;
		buf[++pos] = info->name_len;
		buf[++pos] = (info->id >> 8) & 0xFF;
		buf[++pos] = info->id & 0xFF;
		buf[++pos] = (info->mtu >> 8) & 0xFF;
		buf[++pos] = info->mtu & 0xFF;
		buf[++pos] = (info->dot10 >> 24) & 0xFF;
		buf[++pos] = (info->dot10 >> 16) & 0xFF;
		buf[++pos] = (info->dot10 >> 8) & 0xFF;
		buf[++pos] = info->dot10 & 0xFF;
	
		memcpy(buf+pos+1, info->name, info->len - 12);
		pos += info->len - 12; //+ ((info->len-12)%4? 4 - (info->len-12)%4 : 0);
		
		//printk(KERN_INFO "Vlan name: %s [%d]\n", info->name, pos+1);
	}
}


void vtp_send_subset(struct net_switch* sw, struct vtp_subset* sub)
{
	struct net_switch_port* p;
	unsigned char* buf;
	
	buf = kmalloc(sub->length, GFP_ATOMIC);
	if(!buf) return;
	
	//printk(KERN_INFO "Subset created [%d]\n", sub->length);
	
	memset(buf, 0, sub->length);
	create_subset(sub, buf);

	printk(KERN_INFO "vtp_send_subset: domain [%s] --- sw domain [%s]\n", sub->domain, sw->vtp_vars.domain);
	
	list_for_each_entry(p, &sw->ports, lh) 
	{
		if(p->flags & SW_PFL_TRUNK)
		{	
			//printk(KERN_INFO "Port: %s\n", p->dev->name);
			vtp_send_message(p, buf, sub->length);
		}
	}
	
	kfree(buf);
}

void vtp_send_subsets(struct net_switch* sw, u16 start)
{
	int i = 0;
	struct vtp_subset* sub;
	
	printk(KERN_INFO "vtp_send_subsets: send subsets from %d\n", start);
	
	list_for_each_entry(sub, &sw->vtp_vars.subsets, subsets)
	{
		if(i >= start)
			vtp_send_subset(sw, sub);
		i++;
	}
}

static inline void create_request(struct net_switch* sw, unsigned char* buf)
{
	buf[0] = 0x01;
	buf[1] = VTP_REQUEST;
	buf[2] = 0;
	buf[3] = sw->vtp_vars.dom_len;
	memcpy(buf+4, sw->vtp_vars.domain, sw->vtp_vars.dom_len);
	buf[36] = (sw->vtp_vars.last_subset >> 8) & 0xFF;
	buf[37] = sw->vtp_vars.last_subset & 0xFF;
}

void vtp_send_request(struct net_switch* sw)
{
	unsigned char buf[38];
	struct net_switch_port* p;
	
	printk(KERN_INFO "Send request ---------------------------------------------------------\n");
	
	memset(buf, 0, 38);
	create_request(sw, buf);
	
	list_for_each_entry(p, &sw->ports, lh) 
	{
		if(p->flags & SW_PFL_TRUNK)
		{	
			printk(KERN_INFO "Port: %s\n", p->dev->name);
			vtp_send_message(p, buf, 38);
		}
	}
}


void add_active_vlans(struct net_switch_port *port, u8 *mask)
{
  int i;
  for (i = 0; i<VTP_VLAN_MASK_LG; i++) {
    mask[i] |= port->vtp_vars.active_vlans[i]; 
  }
}

void add_not_punned_vlans(u8 *mask)
{
  bitset(mask, 1);
  bitset(mask, 1002);
  bitset(mask, 1003);
  bitset(mask, 1004);
  bitset(mask, 1005);
}

void print(u8 *mask) {
  int i;

  for (i = 0; i<VTP_VLAN_MASK_LG; i++) {
    printk(KERN_INFO "%02x ", mask[i]);
  }
}

void create_vtp_join(struct net_switch *sw, struct net_switch_port* port, unsigned char* buf)
{
  struct net_switch_port *p;
  u8 vlan_mask[VTP_VLAN_MASK_LG];
  int mask_offset = 4 + sizeof(u32) + VTP_DOMAIN_SIZE;
  int vlan_offset = 4 + VTP_DOMAIN_SIZE;
 
  buf[0] = 0x01;
  buf[1] = VTP_JOIN;
  buf[2] = 0;
  buf[3] = sw->vtp_vars.dom_len;
 
  memset(buf + 4, 0, VTP_DOMAIN_SIZE);
  memcpy(buf + 4, sw->vtp_vars.domain, sw->vtp_vars.dom_len);

  u32 *vlan = (u32 *)(buf + vlan_offset);
  *vlan = htonl(1);

  /* Create mask to send */
  memset(vlan_mask, 0, VTP_VLAN_MASK_LG); 
  list_for_each_entry(p, &sw->ports, lh) {
    if (p != port) {
      add_active_vlans(p, vlan_mask);
    }
  }
  /* Add vlans that are NOT prunning eligible: 1, 1002, 1003, 1004, 1005 */
  add_not_punned_vlans(vlan_mask);
  memcpy(buf + mask_offset, vlan_mask, VTP_VLAN_MASK_LG);
}


void vtp_send_join_port(struct net_switch_port* port) 
{
  int len = 4 + sizeof(u32) + VTP_DOMAIN_SIZE + VTP_VLAN_MASK_LG;
  unsigned char buf[len];

  create_vtp_join(port->sw, port, buf);

  vtp_send_message(port, buf, len);
}


/* Send join frame on all trunk ports */
void vtp_send_join(struct net_switch* sw)
{
  struct net_switch_port* port;
  unsigned char no_domain[VTP_DOMAIN_SIZE];
  
  memset(no_domain, 0, VTP_DOMAIN_SIZE);


  /* Check if vtp is enabled and if a domain is set */
  if (is_vtp_enabled(sw) && 
      sw->vtp_vars.dom_len && 
      memcmp(sw->vtp_vars.domain, no_domain, VTP_DOMAIN_SIZE)) {

    printk(KERN_INFO "Transmite VTP join dupa verificari de domeniu\n");
    list_for_each_entry(port, &sw->ports, lh) {
      if (port->flags &  SW_PFL_TRUNK) {
	vtp_send_join_port(port);
      }
    }
  }
}

struct vtp_summary* create_summary_from_buffer(unsigned char* buf)
{
	struct vtp_summary* sum;

	sum = (struct vtp_summary*) kmalloc (sizeof(struct vtp_summary), GFP_ATOMIC);
	if(!sum)
		return NULL;
	
	sum->version = buf[0];
	sum->code = buf[1];
	sum->followers = buf[2];
	//printk(KERN_INFO "create_summary_from_buffer: sum->followers = %d\n", buf[2]);

	sum->dom_len = buf[3];
	memset(sum->domain, 0, VTP_DOMAIN_SIZE);
	memcpy(sum->domain, buf+4, VTP_DOMAIN_SIZE);
	sum->revision =  buf[36] << 24 |
			buf[37] << 16 |
			buf[38] << 8 |
			buf[39];

	sum->updater = buf[40] << 24 |
			buf[41] << 16 |
			buf[42] << 8 |
			buf[43];

	memcpy(sum->timestamp, buf+44, VTP_TIMESTAMP_SIZE);
	memcpy(sum->md5, buf+44 + VTP_TIMESTAMP_SIZE, VTP_MD5_SIZE);

	return sum;
}


static inline struct vlan_info* create_info_from_buffer(unsigned char* buf)
{
	struct vlan_info* info;
	
	info = (struct vlan_info*) kmalloc (sizeof(struct vlan_info), GFP_ATOMIC);
	if(!info)
		return NULL;
	
	INIT_LIST_HEAD(&info->lh);
	
	info->len = buf[0];
	info->status = buf[1];
	info->type = buf[2];
	info->name_len = buf[3];
	info->id = buf[4] << 8 | buf[5];
	info->mtu = buf[6] << 8 | buf[7];
	info->dot10 = buf[8] << 24 |
			buf[9] << 16 |
			buf[10] << 8 |
			buf[11];
	memset(info->name, 0, SW_MAX_VLAN_NAME+1); 
	memcpy(info->name, buf+12, info->name_len);
	
	return info;
}

struct vtp_subset* create_subset_from_buffer(unsigned char* buf, int len)
{
	struct vtp_subset* sub;
	struct vlan_info* info;
	int pos;
	
	sub = (struct vtp_subset*) kmalloc (sizeof(struct vtp_subset), GFP_ATOMIC);
	if(!sub) 
		return NULL;
	
	INIT_LIST_HEAD(&sub->infos);
	INIT_LIST_HEAD(&sub->subsets);
	
	sub->version = buf[0];
	sub->code = buf[1];
	sub->seq = buf[2];
	sub->dom_len = buf[3];
	memset(sub->domain, 0, VTP_DOMAIN_SIZE);
	memcpy(sub->domain, buf+4, sub->dom_len);
	sub->revision = buf[36] << 24 | 
			buf[37] << 16 | 
			buf[38] << 8 | 
			buf[39];
	
	for(pos = 40; pos < len;)
	{
		info = create_info_from_buffer(buf+pos);
		if(!info)
		{
			destroy_subset(sub);
			return NULL;
		}
		
		list_add(&info->lh, &sub->infos);
		pos += info->len;
	}
	
	printk(KERN_INFO "create_subset_from_buffer: domain rcvd [%s] [%s]\n", sub->domain, buf+4);

	return sub;
}

struct vtp_request* create_request_from_buffer(unsigned char* buf)
{
	struct vtp_request* req;

	req = (struct vtp_request*) kmalloc (sizeof(struct vtp_request), GFP_ATOMIC);
	if(!req)
		return NULL;

	req->version = buf[0];
	req->code = buf[1];
	req->reserved = buf[2];
	memcpy(&req->domain, buf+4, VTP_DOMAIN_SIZE);
	req->start_val = buf[4 + VTP_DOMAIN_SIZE] << 8 |
			buf[5 + VTP_DOMAIN_SIZE];

	return req;	
}

/* 
Get join information 
*/
struct vtp_join* create_join_from_buffer(unsigned char *buf)
{
  struct vtp_join* join;
  int vlan_offset = 4 + VTP_DOMAIN_SIZE;
  int mask_offset = vlan_offset + sizeof(u32);

  join = (struct vtp_join*)kmalloc(sizeof(struct vtp_join), GFP_ATOMIC);
  if (!join)
    return NULL;

  join->version = buf[0];
  join->code = buf[1];
  join->reserved = buf[2];

  memcpy(&join->domain, buf + 4, VTP_DOMAIN_SIZE);

  u32* vlan = (u32*)(buf + vlan_offset);
  join->vlan = ntohl(*vlan);

  memcpy(join->active_vlans, buf + mask_offset, VTP_VLAN_MASK_LG);
      
  return join;
}

void vtp_handle_message(struct sk_buff* sk_b)
{
	unsigned char* buf;
	struct net_switch_port* p = rcu_dereference(sk_b->dev->sw_port);
	struct vtp_summary* summary = NULL;
	struct vtp_request* request = NULL;
	struct vtp_subset* subset = NULL;
	struct vtp_join* join = NULL;
	struct sk_buff* skb = NULL;

	if(!p)
		goto out;
	/*
	if((skb = dev_alloc_skb(sk_b->len)) == NULL)
	{
		printk(KERN_INFO "VTP vtp_send_message: No memory available to create skb %d.\n", sk_b->len);
		return;
	}
	*/
	
	skb = skb_copy(sk_b, GFP_ATOMIC);
	if(!skb)
	{
		printk(KERN_INFO "VTP vtp_send_message: No memory available to create skb %d.\n", sk_b->len);
		return;
	}
	strip_vlan_tag(skb);
	
	// Check if frame contains the LLC VTP header.
	if(!pskb_may_pull(skb, sizeof(vtp_llc_header)) ||
	   memcmp(skb->data, vtp_llc_header, sizeof(vtp_llc_header)))
		goto out;
	
	// Remove form skb->data the LLC VTP header.
	skb_pull(skb, sizeof(vtp_llc_header));

	// Check if frame contains the SNAP VTP header.
	if(!pskb_may_pull(skb, sizeof(vtp_snap_header)) ||
	   memcmp(skb->data, vtp_snap_header, sizeof(vtp_snap_header)))
		goto out;
	
	// Remove form skb->data the SNAP VTP header. 
	buf = skb_pull(skb, sizeof(vtp_snap_header));

	// Check if in skb->data are at least 2 bytes to perfom the checks 
	//   for vtp version and vtp packet type.
	
	if(!pskb_may_pull(skb, 2))
		goto out;
	
	spin_lock_bh(&p->sw->vtp_vars.lock);
	
	// Check packet vtp version.
	if(buf[0] > p->sw->vtp_vars.version)
		goto out;
	
	// Determine packet type and extract the content. 
	switch(buf[1])
	{
		case VTP_SUMM_ADVERT:			
			if(!pskb_may_pull(skb, sizeof(struct vtp_summary)))
			{
				printk(KERN_INFO "VTP summary packet has invalid length.\n");
				goto out;
			}

			summary = create_summary_from_buffer(buf);
			vtp_rcvd_summary(p, summary);	
			break;

		case VTP_SUBSET_ADVERT:
			if(!pskb_may_pull(skb, sizeof(struct vtp_subset) - 2*sizeof(struct list_head) - sizeof(int)))
			{
				printk(KERN_INFO "VTP subset packet has invalid length.\n");
				goto out;
			}

			subset = create_subset_from_buffer(buf, skb->len);	
			vtp_rcvd_subset(p, subset);	
			break;

		case VTP_REQUEST:
			printk(KERN_INFO "VTP request packet\n");
			
			if(!pskb_may_pull(skb, sizeof(struct vtp_request) - 2))
			{
				printk(KERN_INFO "VTP request packet has invalid length.\n");
				goto out;
			}
			
			request = create_request_from_buffer(buf);	
			vtp_rcvd_request(p, request);	
			break;

		case VTP_JOIN:
			if(!pskb_may_pull(skb, 4 + sizeof(u32) + VTP_DOMAIN_SIZE + VTP_VLAN_MASK_LG))
			{
				printk(KERN_INFO "VTP join packet has invalid length.\n");
				goto out;
			}
		
			join = create_join_from_buffer(buf);	
			vtp_rcvd_join(p, join);	
			break;

		default:
			printk(KERN_INFO "Invalid vtp packet.\n");	
			break;
	}
	
out:
	spin_unlock_bh(&p->sw->vtp_vars.lock);
	dev_kfree_skb(skb);
}



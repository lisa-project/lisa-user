#include "sw_private.h"


/*Warning: Do not lock sw->vtp_vars.lock before entering this functions. */
int vtp_set_timestamp(struct net_switch* sw, char* timestamp)
{
	int ret = 0;

	if(timestamp == NULL)
		return -EINVAL;
	
	spin_lock(&sw->vtp_vars.lock);
	if(!is_vtp_server(sw))
		goto out;

	memcpy(sw->vtp_vars.last_update, timestamp, VTP_TIMESTAMP_SIZE);
	
out:
	spin_unlock(&sw->vtp_vars.lock);
	return ret;
}

void sw_vtp_init(struct net_switch* sw)
{
	atomic_set(&sw->vtp_vars.vtp_enabled, 0);
	sw->vtp_vars.password_enabled = 0;
	
	memset(&sw->vtp_vars.password, 0, VTP_MAX_PASSWORD_SIZE);
	memset(&sw->vtp_vars.md5, 0, VTP_MD5_SIZE);
	memset(&sw->vtp_vars.domain, 0, VTP_DOMAIN_SIZE);

	sw->vtp_vars.mode = VTP_MODE_SERVER;
	sw_timer_init(&sw->vtp_vars.summary_timer, sw_summary_timer_expired, (unsigned long) sw);

	sw->vtp_vars.version = VTP_DEFAULT_VERSION;
	sw->vtp_vars.revision = 0;
	memset(&sw->vtp_vars.last_updater, 0, 4);
	memset(&sw->vtp_vars.last_update, 0, VTP_TIMESTAMP_SIZE);
	
	sw->vtp_vars.last_subset = 0;
	
	spin_lock_init(&sw->vtp_vars.lock);
	INIT_LIST_HEAD(&sw->vtp_vars.subsets);

	printk(KERN_INFO "Vtp init.\n");
}

int sw_vtp_enable(struct net_switch* sw, char mode)
{
	int ret = 0;
	
	ret = sw_vtp_set_mode(sw, mode);
	if(ret == 0)
	{
		atomic_set(&sw->vtp_vars.vtp_enabled, 1);
		
		if(is_vtp_server(sw))
		{
			vtp_send_summary(sw, PROCESS_CONTEXT, 0);
			vtp_send_subsets(sw, 0);
		}
	}
	
	printk(KERN_INFO "Vtp enabled.\n");
	
	return ret;
}

void sw_vtp_disable(struct net_switch* sw)
{
	atomic_set(&sw->vtp_vars.vtp_enabled, 0);
	
	if(is_vtp_server(sw))	
		del_timer(&sw->vtp_vars.summary_timer);
}

int sw_vtp_set_domain(struct net_switch* sw, char* domain)
{
	char vtp_domain[VTP_DOMAIN_SIZE+1];
	char max_len = sw->vtp_vars.dom_len;
	char len = 0;

	if(domain == NULL)
		return -EINVAL;

	len = strnlen_user(domain, VTP_DOMAIN_SIZE) - 1;
	if(len < 1|| len > VTP_DOMAIN_SIZE)
		return -EINVAL;
	
	max_len = max_len > len ? max_len : len;
	
	memset(vtp_domain, 0, VTP_DOMAIN_SIZE+1);
	
	printk(KERN_INFO "sw_vtp_set_domain: domain len %d\n", len);
				
	if(strncpy_from_user(vtp_domain, domain, len) != len)
		return -EFAULT;
	
	if(sw->vtp_vars.dom_len && (max_len = sw->vtp_vars.dom_len) && (max_len == len) &&
	  (memcmp(&sw->vtp_vars.domain, vtp_domain, max_len) == 0))
	{
		printk(KERN_INFO "VTP domain is the same.\n");
		return 0;
	}
	
	spin_lock_bh(&sw->vtp_vars.lock);
	
	memcpy(&sw->vtp_vars.domain, vtp_domain, len);
	sw->vtp_vars.dom_len = len;
	if(is_vtp_enabled(sw))
	{
		if(is_vtp_client(sw))
			vtp_send_request(sw);
		else if(is_vtp_server(sw))
		{
			destroy_subset_list(sw);
			sw->vtp_vars.subset_list_size = 0;
			vtp_send_summary(sw, PROCESS_CONTEXT, 0);
			vtp_send_subsets(sw, 0);
		}
	}
	
	spin_unlock_bh(&sw->vtp_vars.lock);
	printk(KERN_INFO "VTP domain changed to %s.\n", sw->vtp_vars.domain);
	
	return 0;
}

int sw_vtp_set_mode(struct net_switch* sw, u8 mode)
{
	
	/* Check the mode value for consistency. */
	if((mode != VTP_MODE_TRANSPARENT) &&
	  (mode != VTP_MODE_CLIENT) &&
	  (mode != VTP_MODE_SERVER))
	{
		//printk(KERN_INFO "CCCCCCCCCCCCCCCCCCCCC mode = %d\n", mode);
		return -EINVAL;
	}
	
	//printk(KERN_INFO "BBBBBBBBBBBBBBBBBBB\n");

	if(is_vtp_enabled(sw) && mode == sw->vtp_vars.mode)
	{
		switch(mode)
		{
			case VTP_MODE_TRANSPARENT:
				printk(KERN_INFO "Switch is already in VTP_MODE_TRANSPARENT\n");
				break;
			case VTP_MODE_CLIENT:
				printk(KERN_INFO "Switch is already in VTP_MODE_CLIENT\n");
				break;
			case VTP_MODE_SERVER:
				printk(KERN_INFO "Switch is already in VTP_MODE_SERVER\n");
				break;
		}
	}

	/* If switch is vtp server and the new mode is different fro vtp server mode 
	   then summary timer must be stopped.
	*/
	if(is_vtp_server(sw) && mode != VTP_MODE_SERVER)
	{
		del_timer(&sw->vtp_vars.summary_timer);
	}
	
	sw->vtp_vars.mode = mode;

	if(is_vtp_server(sw))
	{
		sw->vtp_vars.revision = 1;
		mod_timer(&sw->vtp_vars.summary_timer, jiffies+VTP_SUMMARY_DELAY);
		//printk(KERN_INFO "AAAAAAAAAAAAAAAA\n");
	}
	
	switch(mode)
	{
		case VTP_MODE_TRANSPARENT:
			printk(KERN_INFO "Switch entered VTP_MODE_TRANSPARENT\n");
			break;
		case VTP_MODE_CLIENT:
			printk(KERN_INFO "Switch entered VTP_MODE_CLIENT\n");
			break;
		case VTP_MODE_SERVER:
			printk(KERN_INFO "Switch entered VTP_MODE_SERVER\n");
			break;
	}
	
	return 0;
}


int sw_vtp_set_password(struct net_switch* sw, char* pass, unsigned char* md5)
{
	char len = 0;
	char password[VTP_MAX_PASSWORD_SIZE+1];
	unsigned char md[VTP_MD5_SIZE];
	
	memset(password, 0, VTP_MAX_PASSWORD_SIZE+1);
	memset(md, 0, VTP_MD5_SIZE);

	if(pass == NULL || md5 == NULL)
		return -EINVAL;

	len = strnlen_user(pass, VTP_MAX_PASSWORD_SIZE) - 1;
	if(len < 8 || len > VTP_MAX_PASSWORD_SIZE)
		return -EINVAL;
	
	if ((strncpy_from_user(password, pass, len) != len) ||
            copy_from_user(md, md5, VTP_MD5_SIZE))
		return -EFAULT;
	
	spin_lock_bh(&sw->vtp_vars.lock);
	memcpy(sw->vtp_vars.password, password, len);
	memcpy(sw->vtp_vars.md5, md, VTP_MD5_SIZE);
	sw->vtp_vars.password_enabled = 1;
	spin_unlock_bh(&sw->vtp_vars.lock);
	
	printk(KERN_INFO "Password set to %s.\n", sw->vtp_vars.password);
	
	return 0;
}

int sw_vtp_set_version(struct net_switch* sw, char version)
{
	if((version != 1) && (version != 2))	
		return -EINVAL;
	
	sw->vtp_vars.version = version;
	
	printk(KERN_INFO "Vtp version became %d.\n", sw->vtp_vars.version);
	
	return 0;
}

int vtp_pruning_enable(struct net_switch* sw)
{
	sw->vtp_vars.pruning_enabled = 1;
	printk(KERN_INFO "Pruning switched on.\n");

	return 0;
}

int vtp_pruning_disable(struct net_switch* sw)
{
	sw->vtp_vars.pruning_enabled = 0;
	printk(KERN_INFO "Pruning switched off.\n");
	
	return 0;
}

static inline int check_domain(struct net_switch* sw, u8* domain)
{
	static u8 no_domain[VTP_DOMAIN_SIZE];
	memset(no_domain, 0, VTP_DOMAIN_SIZE);
	
	printk(KERN_INFO "check_domain: switch domain [%s] ----- domain: [%s]\n", sw->vtp_vars.domain, domain);
	
	if(!memcmp(sw->vtp_vars.domain, domain, VTP_DOMAIN_SIZE))
		return 1;
	
	if(!sw->vtp_vars.dom_len && (memcmp(no_domain, domain, VTP_DOMAIN_SIZE) != 0))
	{
		printk(KERN_INFO "check_domain: send request ----- new domain: %s\n", domain);
		vtp_send_request(sw);
		return 1;
	}
		
	return 0;
}

static inline int check_password(struct net_switch* sw, struct vtp_summary* sum)
{
	if(!memcmp(sw->vtp_vars.md5, sum->md5, VTP_MD5_SIZE))
		return 1;
		
	return 0;
}

static inline int check_revision(struct net_switch* sw, struct vtp_summary* sum)
{
	if(sum->followers && !sw->vtp_vars.followers && 
	  (sw->vtp_vars.revision == sum->revision))	
		return 1;
	if(sw->vtp_vars.revision < sum->revision)	
		return 1;
	if((sw->vtp_vars.revision > sum->revision) &&
	  (memcmp(sw->vtp_vars.last_update, sum->timestamp, VTP_TIMESTAMP_SIZE) < 0))
		return 1;
	
	return 0;
}

static inline void update_vtp_info(struct net_switch* sw, struct vtp_summary* sum)
{
        /* Change domain name if necesary */
        if (memcmp(sw->vtp_vars.domain, sum->domain, VTP_DOMAIN_SIZE))
	{
	  sw->vtp_vars.dom_len = sum->dom_len;
	  memcpy(sw->vtp_vars.domain, sum->domain, VTP_DOMAIN_SIZE);
	  /* Domain has changed - send vtp join messages an all trunk ports */
	  vtp_send_join(sw);
	}
	
	sw->vtp_vars.revision = sum->revision;
	memset(sw->vtp_vars.last_update, 0, VTP_TIMESTAMP_SIZE + 1);
	memcpy(sw->vtp_vars.last_update, sum->timestamp, VTP_TIMESTAMP_SIZE);
	
	sw->vtp_vars.followers = sum->followers;
	sw->vtp_vars.last_subset = 0;
}

void vtp_rcvd_summary(struct net_switch_port* p, struct vtp_summary* sum)
{
	//int i;
	int ret;
	
	printk(KERN_INFO "vtp_rcvd_summary:\n");
	if(!sum)
	{
		printk(KERN_INFO "NULL summary\n");
		return;
	}
	
	/*
	struct vtp_summary 
	{
		u8  version;
		u8  code;
		u8  followers; 
		u8  dom_len;
		u8  domain[VTP_DOMAIN_SIZE];
		u32 revision;
		u32 updater;
		u8  timestamp[VTP_TIMESTAMP_SIZE];
		u8  md5[VTP_MD5_SIZE];
	};
	*/
	
	/*
	printk(KERN_INFO "- version = %d \n", sum->version);
	printk(KERN_INFO "- code = %d \n", sum->code);
	printk(KERN_INFO "- followers = %d \n", sum->followers);
	printk(KERN_INFO "- domain length = %d \n", sum->dom_len);
	printk(KERN_INFO "- domain = %s \n", sum->domain);
	printk(KERN_INFO "- revision = %d \n", sum->revision);
	printk(KERN_INFO "- updater = %d.%d.%d.%d \n", 
	       (sum->updater >> 24) & 0xFF, 
	       (sum->updater >> 16) & 0xFF, 
	       (sum->updater >> 8) & 0xFF, 
	       sum->updater & 0xFF);
	
	printk(KERN_INFO "- timestamp :");
	for(i = 0; i < VTP_TIMESTAMP_SIZE; i++)
		printk(KERN_INFO "%x", sum->timestamp[i]);
	printk(KERN_INFO "-  md5:");
	for(i = 0; i < VTP_MD5_SIZE; i++)
		printk(KERN_INFO "%x", sum->md5[i]);
	*/
		
	ret = check_domain(p->sw, sum->domain);
	if(!ret)
	{ 
		printk(KERN_INFO "vtp_rcvd_summary: Invalid domain\n");
		goto free;
	}
	
	ret = check_password(p->sw, sum);
	if(!ret)
	{ 
		printk(KERN_INFO "vtp_rcvd_summary: Invalid password\n");
		goto free;
	}
	
	ret = check_revision(p->sw, sum);
	if(!ret)
	{ 
		printk(KERN_INFO "vtp_rcvd_summary: I have better revision\n");
		goto free;
	}	
	
	update_vtp_info(p->sw, sum);
	
	if(!sum->followers)
		vtp_send_request(p->sw);
	
free:
	kfree(sum);
}

static inline void update_vdb(struct net_switch* sw, struct vtp_subset* sub)
{
	int i, ret, found;
	struct vlan_info* info;
	
	printk(KERN_INFO "update_vdb.");
	
	for(i = 1; i <= SW_MAX_VLAN; i++)
	{		
		found = 0;
		
		list_for_each_entry(info, &sub->infos, lh)
		{
			if(i == info->id)
			{
				if(!sw->vdb[i])
				{
					ret = sw_vdb_add_vlan(sw, i, info->name);
					if(ret < 0)
						printk(KERN_INFO "VLAN[%d][%s] was not added: error = %d\n",
							i, info->name, ret);
					found = 1;
					break;
				}
				else
				{
					ret = sw_vdb_set_vlan_name(sw, i, info->name);
					if(ret < 0)
						printk(KERN_INFO "VLAN[%d][%s] name could not be set: error = %d\n",
							i, info->name, ret);
					found = 1;
					break;
				}
					
			}
		}
		
		if(!found && sw->vdb[i])
		{
			ret = sw_vdb_del_vlan_irq(sw, i);
			if(ret < 0)
				printk(KERN_INFO "VLAN[%d][%s] could not be deleted: error = %d\n", i, sw->vdb[i]->name, ret);	
		}
	}
	
	
}

void vtp_rcvd_subset(struct net_switch_port* p, struct vtp_subset* sub)
{
	int ret;
	
	
	printk(KERN_INFO "vtp_rcvd_subset:\n");
	
	if(!sub) 
	{
		printk(KERN_INFO "NULL subset\n");
		return;
	}

	/*		
	printk(KERN_INFO "- version = %d \n", sub->version);
	printk(KERN_INFO "- code = %d \n", sub->code);
	printk(KERN_INFO "- seq = %d \n", sub->seq);
	printk(KERN_INFO "- domain length = %d \n", sub->dom_len);
	printk(KERN_INFO "- domain = %s \n", sub->domain);
	printk(KERN_INFO "- revision = %d \n", sub->revision);
	*/

	ret = check_domain(p->sw, sub->domain);
	if(!ret)
	{ 
		printk(KERN_INFO "vtp_rcvd_subset: Invalid domain\n");
		goto free;
	}
	
	if(p->sw->vtp_vars.revision != sub->revision)
	{
		printk(KERN_INFO "vtp_rcvd_subset: Not the revision number expected.\n");
		goto free;
	}
	
	if(!p->sw->vtp_vars.followers)
	{
		printk(KERN_INFO "vtp_rcvd_subset: No subset is expected.\n");
		goto free;
	}
	
	if((p->sw->vtp_vars.last_subset + 1) != sub->seq)
	{
		printk(KERN_INFO "vtp_rcvd_subset: Not the subset expected -> send request.\n");
		vtp_send_request(p->sw);
		goto free;
	}
	
	/* Update vlan database.*/
	update_vdb(p->sw, sub);
	
	p->sw->vtp_vars.last_subset = sub->seq;
	p->sw->vtp_vars.followers--;
	
	if(!p->sw->vtp_vars.followers)
	{
		p->sw->vtp_vars.followers = 0;
		p->sw->vtp_vars.last_subset = 0;
	}
	
free:	
	destroy_subset(sub);
}

void vtp_rcvd_request(struct net_switch_port* p, struct vtp_request* req)
{
	int ret;
	printk(KERN_INFO "vtp_rcvd_request:\n");
	
	if(!req) 
	{
		printk(KERN_INFO "NULL request\n");
		return;
	}
	
	if(!is_vtp_server(p->sw))
		goto free;
	
	/*
	struct vtp_request 
	{
		u8  version;
		u8  code;
		u8  reserved;
		u8  dom_len;
		u8  domain[VTP_DOMAIN_SIZE];
		u16 start_val;
	};	
	*/
	
	printk(KERN_INFO "- version = %d \n", req->version);
	printk(KERN_INFO "- code = %d \n", req->code);
	printk(KERN_INFO "- reserved  = %d \n", req->reserved);
	printk(KERN_INFO "- domain legth = %d \n", req->dom_len);
	printk(KERN_INFO "- domain = %s \n", req->domain);
	printk(KERN_INFO "- start val = %d \n", req->start_val);
	
	ret = check_domain(p->sw, req->domain);
	if(!ret)
	{
		printk(KERN_INFO "vtp_rcvd_request: Request from invalid domain.\n");
		goto free;
	}
	
	vtp_send_summary(p->sw, PROCESS_CONTEXT, req->start_val);
	vtp_send_subsets(p->sw, req->start_val);
	
free:	
	kfree(req);	
}

void vtp_rcvd_join(struct net_switch_port* p, struct vtp_join* join)
{
	int i;
	printk(KERN_INFO "vtp_rcvd_join\n");
	
	if(check_domain(p->sw, join->domain))
	{
		printk(KERN_INFO "vtp_rcvd_join: Invalid domain\n");
		return;
	}
	
	for(i = 0; i < VTP_VLAN_MASK_LG; i++)
		p->vtp_vars.active_vlans[i] |= join->active_vlans[i];
	
	kfree(join);
}



#include "sw_private.h" 
#include <linux/ethtool.h>

/*
	Warning: Do not call this function under interrupt context. 
		 dev_ethtool is a sleeping function.
*/
void sw_detect_port_speed(struct net_switch_port* p)
{
	struct ethtool_cmd ecmd = {ETHTOOL_GSET};
	struct ifreq ifr;
	mm_segment_t old_fs;
	int err;

	memcpy(ifr.ifr_name, p->dev->name, IFNAMSIZ);
	ifr.ifr_data = (void __user *) &ecmd;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = dev_ethtool(&ifr);
	set_fs(old_fs);
	
	if(err < 0)
	{
		printk("dev_ethtool: err = %d for port = %s\n", err, p->dev->name);
		p->stp_vars.path_cost = SPEED_10_COST;
		return;
	}

	switch(ecmd.speed)
	{
		case SPEED_10:
			p->stp_vars.path_cost = SPEED_10_COST;	
			printk("Port %s: speed 10 (%d)\n", 
				p->dev->name,
				p->stp_vars.path_cost);
			return;
		case SPEED_100:
			p->stp_vars.path_cost = SPEED_100_COST;	
			printk("Port %s: speed 100 (%d)\n", 
				p->dev->name,
				p->stp_vars.path_cost);
			return;
		case SPEED_1000:
			p->stp_vars.path_cost = SPEED_1000_COST;	
			printk("Port %s: speed 1000 (%d)\n", 
				p->dev->name,
				p->stp_vars.path_cost);
			return;
		case SPEED_10000:
			p->stp_vars.path_cost = SPEED_10000_COST;	
			printk("Port %s: speed 10000 (%d)\n", 
				p->dev->name,
				p->stp_vars.path_cost);
			return;
		default:
			p->stp_vars.path_cost = SPEED_10_COST;	
			printk("Port %s: speed asumed 10 (%d)\n", 
				p->dev->name,
				p->stp_vars.path_cost);
			return;
	}
}

static inline port_id make_port_id(u8 priority ,u16 port_no)
{
	return ((u16)priority << PORT_BITS) | 
		(port_no & ((1 << PORT_BITS) - 1)); 
}

void sw_make_port_id(struct net_switch_port* p)
{
	int i; 
	u16 port_no;
	char* pos = NULL;
			
	pos = p->dev->name + 3;
	port_no = 0;
	
	for(i = 0; *(pos + i) != '\0'; i++)
	{
		port_no = port_no * 10 + *(pos + i) - 0x30;
	}
		
	port_no++;
		
	p->stp_vars.port_id = make_port_id(p->stp_vars.priority, port_no);
	
	printk(KERN_INFO "Dev %s: has id %u", p->dev->name, p->stp_vars.port_id);
}

void sw_stp_init_port(struct net_switch_port* p)
{
	sw_stp_port_timers_init(p);
	
	p->stp_vars.priority = DEFAULT_PORT_PRIORITY;
	sw_make_port_id(p);
	
	become_designated_port(p);
	
	//p->stp_vars.state = STP_STATE_BLOCKING;
	set_port_state(p, STP_STATE_BLOCKING);
	
	p->stp_vars.topology_change_acknowledge = 0;
	p->stp_vars.config_pending = 0;
	p->stp_vars.change_detection_enabled = 1;
}


void update_ports_info(struct net_switch* sw, const unsigned char* old_bridge_mac)
{
	struct net_switch_port* p;
	
	list_for_each_entry(p, &sw->ports, lh)
	{
		if(memcmp(p->stp_vars.designated_bridge.addr, old_bridge_mac, ETH_ALEN) == 0)
			memcpy(p->stp_vars.designated_bridge.addr, sw->stp_vars.bridge_id.addr, ETH_ALEN);
		
		if(memcmp(p->stp_vars.designated_root.addr, old_bridge_mac, ETH_ALEN) == 0)
			memcpy(p->stp_vars.designated_root.addr, sw->stp_vars.bridge_id.addr, ETH_ALEN);
	}
}

void update_bridge_info_from_port(struct net_switch_port* p)
{
	const unsigned char zero_mac[ETH_ALEN];
	unsigned char old_mac[ETH_ALEN];
	struct net_switch* sw = p->sw;	
	int was_root = is_root_bridge(sw);
	
	memset(&zero_mac, 0, ETH_ALEN);
	memcpy(&old_mac, &sw->stp_vars.bridge_id.addr, ETH_ALEN);
	

	if((memcmp(&zero_mac, &sw->stp_vars.bridge_id.addr, ETH_ALEN) == 0) ||
	  (memcmp(&p->dev->dev_addr, &sw->stp_vars.bridge_id.addr, ETH_ALEN) < 0))
	{
		memcpy(&sw->stp_vars.bridge_id.addr, p->dev->dev_addr, ETH_ALEN);
		update_ports_info(sw, old_mac);
	}
		
	configuration_update(sw);
	port_state_selection(sw);

	if(is_root_bridge(sw) && !was_root)
	{
		become_root_bridge(sw);
	}
}

void sw_stp_enable_port(struct net_switch_port* p)
{
	update_bridge_info_from_port(p);
	sw_stp_init_port(p);
	port_state_selection(p->sw);
}

void sw_stp_disable_port(struct net_switch_port* p)
{
	struct net_switch* sw;
	int was_root;
	
	sw = p->sw;
	
	was_root = is_root_bridge(sw);
	
	become_designated_port(p);
	set_port_state(p, STP_STATE_DISABLED);
	p->stp_vars.topology_change_acknowledge = 0;
	p->stp_vars.config_pending = 0;
	
	/* stop all port timers */
	del_timer_sync(&p->stp_vars.message_age_timer);
	del_timer_sync(&p->stp_vars.forward_delay_timer);
	del_timer_sync(&p->stp_vars.hold_timer);
	
	configuration_update(sw);
	port_state_selection(sw);
	
	if(is_root_bridge(sw) && !was_root)
	{
		become_root_bridge(sw);
	}
}


void set_port_priority(struct net_switch_port* p, u8 priority)
{
	port_id new_port_id;
	u16 port_no = (u16)p->stp_vars.port_id & 0xFF;
	
	new_port_id = make_port_id(priority, port_no);
	
	if(is_designated_port(p))
		p->stp_vars.designated_port = new_port_id;
	
	p->stp_vars.port_id = new_port_id;
	
	if(!memcmp(&p->sw->stp_vars.bridge_id, &p->stp_vars.designated_bridge, sizeof(bridge_id)) &&
	  (p->stp_vars.port_id < p->stp_vars.designated_port))
	{
		become_designated_port(p);
		port_state_selection(p->sw);
	}
}


void set_port_cost(struct net_switch_port* port, u32 path_cost) {
  port->stp_vars.path_cost = path_cost;

  configuration_update(port->sw);
  port_state_selection(port->sw);  
}

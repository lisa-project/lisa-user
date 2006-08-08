#include "sw_private.h"

static inline void print_bridge_id(bridge_id *id)
{
	printk(KERN_INFO "%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x\n",
	       id->prio[0], id->prio[1],
	       id->addr[0], id->addr[1], id->addr[2],
	       id->addr[3], id->addr[4], id->addr[5]);
}

void sw_stp_timers_values_init(struct net_switch* sw)
{
	sw->stp_vars.bridge_max_age = DEFAULT_MAX_AGE * HZ;
	sw->stp_vars.ageing_time = DEFAULT_AGEING_TIME * HZ;
	sw->stp_vars.bridge_hello_time = DEFAULT_HELLO_TIME * HZ;
	sw->stp_vars.bridge_forward_delay = DEFAULT_FORWARD_DELAY * HZ;
	
	sw->stp_vars.max_age = sw->stp_vars.bridge_max_age;
	sw->stp_vars.hello_time = sw->stp_vars.bridge_hello_time;
	sw->stp_vars.forward_delay = sw->stp_vars.bridge_forward_delay;
}

void sw_stp_enable(struct net_switch* sw)
{
	struct net_switch_port *p;
	
	atomic_set(&sw->stp_vars.stp_enabled, 1);

	spin_lock_bh(&sw->stp_vars.lock);
	
	sw_stp_timers_values_init(sw);
	sw_stp_timers_init(sw);
	
	sw->stp_vars.bridge_id.prio[0] = (DEFAULT_BRIDGE_PRIORITY >> 8) & 0xFF;
	sw->stp_vars.bridge_id.prio[1] = DEFAULT_BRIDGE_PRIORITY & 0xFF;
	memset(&sw->stp_vars.bridge_id.addr, 0, ETH_ALEN);
	
	memcpy(&sw->stp_vars.designated_root, &sw->stp_vars.bridge_id, sizeof(bridge_id));
	
	sw->stp_vars.root_path_cost = 0;
	sw->stp_vars.root_port = NULL;
	
	//sw->stp_vars.stp_enabled = 1;
	sw->stp_vars.topology_change = 0;
	sw->stp_vars.topology_change_detected = 0;
	
	//del_timer(&sw->stp_vars.tcn_timer);
	//del_timer(&sw->stp_vars.topology_change_timer);

	list_for_each_entry(p, &sw->ports, lh)
		if ((p->dev->flags & IFF_UP) && netif_carrier_ok(p->dev))
		{	
			sw_stp_enable_port(p);
			printk(KERN_INFO "Stp enabled for a port");
		}
	
	port_state_selection(sw);
	config_bpdu_generation(sw);

	mod_timer(&sw->stp_vars.hello_timer, jiffies + sw->stp_vars.hello_time);
	
	/* Nu apare in standard dar apare in net/bridge */
	//mod_timer(&sw->stp_vars.gc_timer, jiffies + HZ/10);

	spin_unlock_bh(&sw->stp_vars.lock);
	
	printk(KERN_INFO "Stp enabled\n");
}

void sw_stp_disable(struct net_switch* sw)
{
	struct net_switch_port* p;

	atomic_set(&sw->stp_vars.stp_enabled, 0);

	spin_lock_bh(&sw->stp_vars.lock);
	
	list_for_each_entry(p, &sw->ports, lh)
	{	
		sw_stp_disable_port(p);
		printk(KERN_INFO "Stp disabled for a port");
	}
	
	spin_unlock_bh(&sw->stp_vars.lock);
	
	/* stop bridge timers */
	del_timer_sync(&sw->stp_vars.hello_timer);
	del_timer_sync(&sw->stp_vars.tcn_timer);
	del_timer_sync(&sw->stp_vars.topology_change_timer);
	del_timer_sync(&sw->stp_vars.gc_timer);

	printk(KERN_INFO "Stp disabled\n");
}

void become_designated_port(struct net_switch_port* p)
{
	memcpy(&p->stp_vars.designated_root, &p->sw->stp_vars.designated_root, sizeof(bridge_id));
	p->stp_vars.designated_cost = p->sw->stp_vars.root_path_cost;
	memcpy(&p->stp_vars.designated_bridge, &p->sw->stp_vars.bridge_id, sizeof(bridge_id));
	p->stp_vars.designated_port = p->stp_vars.port_id;
}

/* Generates config bdpus when hello timer expires */
void config_bpdu_generation(struct net_switch* sw) {
	struct net_switch_port* p;
	
	list_for_each_entry(p, &sw->ports, lh) 
	{
		/*
		printk(KERN_INFO "port %s: test if disabled(%d) or designated(%d)\n",
			p->dev->name, !is_disabled_port(p), is_designated_port(p));
		*/
		
		if(!is_disabled_port(p) && is_designated_port(p))
		{
			transmit_config(p);
		}
	}
}

/* Transmit one config bpdu for a port*/
void transmit_config(struct net_switch_port* p)
{
	//printk(KERN_INFO "Port %u: start transmit config bpdu\n" , p->stp_vars.port_id);
	
	struct sw_stp_bpdu bpdu;
	
	
	if(timer_pending(&p->stp_vars.hold_timer))
	{
		p->stp_vars.config_pending = 1;
		return;
	}
	
	memcpy(&bpdu.root_id, &p->sw->stp_vars.designated_root, sizeof(bridge_id));
	bpdu.root_path_cost = p->sw->stp_vars.root_path_cost;
	memcpy(&bpdu.bridge_id, &p->sw->stp_vars.bridge_id, sizeof(bridge_id));
	memcpy(&bpdu.port_id, &p->stp_vars.port_id, sizeof(port_id));
	

	if(is_root_bridge(p->sw))
	{
		bpdu.message_age = 0;
	}
	else
	{
		bpdu.message_age = p->sw->stp_vars.max_age - 
				(p->sw->stp_vars.root_port->stp_vars.message_age_timer.expires - jiffies) + 
				MESSAGE_AGE_INCREMENT;
	}
	
	
	
	bpdu.max_age = p->sw->stp_vars.max_age;
	bpdu.hello_time = p->sw->stp_vars.hello_time;
	bpdu.forward_delay = p->sw->stp_vars.forward_delay;
	bpdu.flags = (p->sw->stp_vars.topology_change ? 0x80 : 0) |
			(p->stp_vars.topology_change_acknowledge ? 0x01 : 0);
	
	if(bpdu.message_age < p->sw->stp_vars.max_age)
	{
		p->stp_vars.topology_change_acknowledge = 0;
		p->stp_vars.config_pending = 0;
		send_config_bpdu(p, &bpdu);
		mod_timer(&p->stp_vars.hold_timer, jiffies + HOLD_TIME);//p->sw->stp_vars.hold_time);
	}
	else
	{
		printk("transmit_config: message_age = %lu > max_age = %lu\n", bpdu.message_age, p->sw->stp_vars.max_age);
	}
}

void transmit_tcn(struct net_switch* sw) 
{
	//printk(KERN_INFO "transmit_tcn stub");
	
	if(sw->stp_vars.root_port)
		send_tcn_bpdu(sw->stp_vars.root_port);
}


/* Updates information held by the bridge and the bridge ports */
void  configuration_update(struct net_switch* sw) 
{
      root_selection(sw);
  
      designated_port_selection(sw);
}


/* Determines if port can become root */
int can_be_root(struct net_switch_port *port, struct net_switch *sw, struct net_switch_port* root_port)
{
  if (is_designated_port(port)) 
    return 0;
  if (port->stp_vars.state == STP_STATE_DISABLED)
    return 0;
  if (bridge_id_cmp(&port->stp_vars.designated_root, &sw->stp_vars.bridge_id) >= 0)
    return 0;

  /* here the following condition is true 
     (!designated_port(port_no)) && 
     (port_info[port_no].state != Disabled) && 
     (port_info[port_no].designated_root < bridge_info.bridge_id)
  */
  if (root_port == NULL)
    return 1;

  /* Choose using designated_root */
  int cond = bridge_id_cmp(&port->stp_vars.designated_root, &root_port->stp_vars.designated_root);
  if (cond < 0)
    return 1;
  else if (cond > 0)
    return 0;

  /* here port->stp_vars.designated_root = root_port->stp_vars.designated_root */ 

  /* Choose using Root Path Cost */
  u32 port_rcost = port->stp_vars.designated_cost + port->stp_vars.path_cost;
  u32 root_port_rcost = root_port->stp_vars.designated_cost + root_port->stp_vars.path_cost;
  if(port_rcost < root_port_rcost)
    return 1;
  else if (port_rcost > root_port_rcost)
    return 0;
  
  /* here port.root_path_cost = root_port.root_path_cost 
     (where root_path_cost = path_cost + designated_cost) */

  /* Choose using designated bridge */
  cond = bridge_id_cmp(&port->stp_vars.designated_bridge, &root_port->stp_vars.designated_bridge);
  if (cond < 0)
    return 1;
  else if (cond > 0) 
    return 0;

  /* Here port_info[port_no].designated_bridge = port_info[root_port].designated_bridge */
  
  /* Choose using designate_port */
  if (port->stp_vars.designated_port < root_port->stp_vars.designated_port)
    return 1;
  else if (port->stp_vars.designated_port > root_port->stp_vars.designated_port)
    return 0;

  /* Here port_info[port_no].designated_port == port_info[root_port].designated_port */

  /* Choose using port_id */
  if (port->stp_vars.port_id < root_port->stp_vars.port_id)
    return 1;


  return 0;
}


/*
 * Determin designated root, root port
 * and root path cost for bridge
 */
void root_selection(struct net_switch* sw)
{
  //printk(KERN_INFO "ROOT selction stub");

  struct net_switch_port* root_port = NULL;
  struct net_switch_port* port;

  list_for_each_entry(port, &sw->ports, lh) 
  {
    if (can_be_root(port, sw, root_port)) 
    {
      root_port = port;
    }
  }

  sw->stp_vars.root_port = root_port;

  if (root_port == NULL) 
  {
	  //printk(KERN_INFO "root_selection: ROOT_PORT == NULL\n");
    sw->stp_vars.designated_root = sw->stp_vars.bridge_id;
    sw->stp_vars.root_path_cost = 0;
  }
  else
  {
    memcpy(&sw->stp_vars.designated_root, &root_port->stp_vars.designated_root, sizeof(bridge_id));
    sw->stp_vars.root_path_cost = root_port->stp_vars.designated_cost + root_port->stp_vars.path_cost;
  } 
}


/* Test if the port should become designated port */
int can_be_desingnated(struct net_switch_port* port, struct net_switch* sw)
{
  if (is_designated_port(port))
    return 1;

  if (bridge_id_cmp(&port->stp_vars.designated_root, &sw->stp_vars.designated_root) != 0)
     return 1;

  if (sw->stp_vars.root_path_cost < port->stp_vars.designated_cost)
    return 1;
  else if (sw->stp_vars.root_path_cost > port->stp_vars.designated_cost)
    return 0;

  /* Here  bridge_info.root_path_cost == port_info[port_no].designated_cost */

  /* Choose using designated bridge */
  if (bridge_id_cmp(&sw->stp_vars.bridge_id, &port->stp_vars.designated_bridge) < 0)
    return 1;
  else if (bridge_id_cmp(&sw->stp_vars.bridge_id, &port->stp_vars.designated_bridge) > 0)
    return 0;

  /* Here bridge_info.bridge_id == port_info[port_no].designated_bridge */
  
  /* Choose using designated port id */
  if (port->stp_vars.port_id <= port->stp_vars.designated_port)
    return 1;
     
  return 0;
}



/*
 * Determines if the port should be designated 
 * for the LAN to whitch the port is attached
 */
void designated_port_selection(struct net_switch* sw)
{
  //printk(KERN_INFO "Designated port selection");

  struct net_switch_port* port;

  
  list_for_each_entry(port, &sw->ports, lh)
  {
    if (can_be_desingnated(port, sw)) 
    {
      become_designated_port(port);
    }
  }
  
}

void make_forwarding(struct net_switch_port* p)
{
	if(p->stp_vars.state == STP_STATE_BLOCKING)
	{
		set_port_state(p, STP_STATE_LISTENING);
		mod_timer(&p->stp_vars.forward_delay_timer, 
			  jiffies + p->sw->stp_vars.forward_delay);
		
		printk(KERN_INFO "Port %d: entered STP_STATE_LISTENING\n", p->stp_vars.port_id & 0xFF);
	}
}

void make_blocking(struct net_switch_port* p)
{
	if((p->stp_vars.state != STP_STATE_DISABLED)&&
	   (p->stp_vars.state != STP_STATE_BLOCKING))
	{
		if((p->stp_vars.state == STP_STATE_FORWARDING)||
		   (p->stp_vars.state == STP_STATE_LEARNING))
		{
			if (p->stp_vars.change_detection_enabled == 1)
			{
				topology_change_detection(p->sw);
			}
		}

		set_port_state(p, STP_STATE_BLOCKING);
		del_timer(&p->stp_vars.forward_delay_timer);
		
		printk(KERN_INFO "Port %d: entered STP_STATE_BLOCKING\n", p->stp_vars.port_id & 0xFF);
	}
}

void port_state_selection(struct net_switch* sw) 
{
	struct net_switch_port* p;

	//printk(KERN_INFO "Port state selection\n");

	list_for_each_entry(p, &sw->ports, lh)
	{
		
		//printk(KERN_INFO "p = %ul\n", p);
		//printk(KERN_INFO "sw->stp_vars.root_port = %ul\n", sw->stp_vars.root_port);
		
		//if(memcmp(&p->stp_vars.port_id, &sw->stp_vars.root_port->stp_vars.port_id, sizeof(port_id)) == 0)
		//if(memcmp(p, sw->stp_vars.root_port, sizeof(struct net_switch_port)) == 0)
		if(p == sw->stp_vars.root_port)
		{
			p->stp_vars.config_pending = 0;
			p->stp_vars.topology_change_acknowledge = 0;
			
			make_forwarding(p);
			/*
			printk(KERN_INFO "Port %d(root): port_state_selection -> state = %d\n", 
				p->stp_vars.port_id & 0xFF,
				p->stp_vars.state);
			*/
		}
		else if(is_designated_port(p))
		{
			del_timer(&p->stp_vars.message_age_timer);
			make_forwarding(p);
			/*
			printk(KERN_INFO "Port %d(designated): port_state_selection -> state = %d\n", 
			       p->stp_vars.port_id & 0xFF,
			       p->stp_vars.state);
			*/
		}
		else 
		{
			p->stp_vars.config_pending = 0;
			p->stp_vars.topology_change_acknowledge = 0;
			
			make_blocking(p);
			/*
			printk(KERN_INFO "Port %d(not-designated): port_state_selection -> state = %d\n", 
			       p->stp_vars.port_id & 0xFF,
			       p->stp_vars.state);
			*/
		}
	}
}


void topology_change_detection(struct net_switch* sw)
{
  //printk(KERN_INFO "Topology change detection");

  if (is_root_bridge(sw))
  {
    sw->stp_vars.topology_change_detected = 1;
    mod_timer(&sw->stp_vars.topology_change_timer, jiffies + topology_chage_time(sw));
  }
  else if (sw->stp_vars.topology_change_detected == 0)
  {
    transmit_tcn(sw);
    mod_timer(&sw->stp_vars.tcn_timer, jiffies + sw->stp_vars.bridge_hello_time);
  }

  sw->stp_vars.topology_change_detected = 1;
}

static int supersedes_port_info(struct net_switch_port* p, struct sw_stp_bpdu* bpdu)
{
	int cmp;
	
	cmp = bridge_id_cmp(&bpdu->root_id, &p->stp_vars.designated_root);
	if(cmp < 0)
		return 1;
	else if(cmp > 0)
		return 0;
	
	if(bpdu->root_path_cost < p->stp_vars.designated_cost)
		return 1;
	else if(bpdu->root_path_cost > p->stp_vars.designated_cost)
		return 0;

	cmp = bridge_id_cmp(&bpdu->bridge_id, &p->stp_vars.designated_bridge);
	if(cmp < 0)
		return 1;
	else if(cmp > 0)
		return 0;

	cmp = bridge_id_cmp(&bpdu->bridge_id, &p->sw->stp_vars.bridge_id);
	if((cmp != 0) || (bpdu->port_id <= p->stp_vars.designated_port))
		return 1;
		
	return 0;
}

static void record_config_information(struct net_switch_port* p, struct sw_stp_bpdu* bpdu)
{
	memcpy(&p->stp_vars.designated_root, &bpdu->root_id, sizeof(bridge_id));
	p->stp_vars.designated_cost = bpdu->root_path_cost;
	memcpy(&p->stp_vars.designated_bridge, &bpdu->bridge_id, sizeof(bridge_id));
	p->stp_vars.designated_port = bpdu->port_id;
	
	/*
	printk(KERN_INFO "record_config_information -> recorded\n");
	printk(KERN_INFO "- designated_root:");	
	print_bridge_id(&p->stp_vars.designated_root);
	printk(KERN_INFO "- root_path_cost = %d \n", p->stp_vars.designated_cost);
	printk(KERN_INFO "- designated_bridge:");	
	print_bridge_id(&p->stp_vars.designated_bridge);
	printk(KERN_INFO "- designated_portport = %u\n", p->stp_vars.designated_port && 0xFF);
	*/
	mod_timer(&p->stp_vars.message_age_timer, jiffies + p->sw->stp_vars.max_age - bpdu->message_age);
}

static inline void record_config_timeout_values(struct net_switch* sw, struct sw_stp_bpdu* bpdu)
{
	sw->stp_vars.max_age = bpdu->max_age;
        sw->stp_vars.hello_time = bpdu->hello_time;
        sw->stp_vars.forward_delay = bpdu->forward_delay;
        sw->stp_vars.topology_change = (bpdu->flags & 0x80) ? 1 : 0;
}

static inline void topology_change_acknowledged(struct net_switch* sw)
{
	sw->stp_vars.topology_change_detected = 0;
	del_timer(&sw->stp_vars.tcn_timer);
	
	//printk(KERN_INFO "topology_change_acknowledged\n.");
}


static inline void reply(struct net_switch_port* p)
{
	transmit_config(p);
}

static inline void print_bpdu(struct sw_stp_bpdu* bpdu)
{
	printk(KERN_INFO "- root_id:");	
	print_bridge_id(&bpdu->root_id);
	printk(KERN_INFO "- root_path_cost = %d \n", bpdu->root_path_cost);
	printk(KERN_INFO "- bridge_id:");	
	print_bridge_id(&bpdu->bridge_id);
	printk(KERN_INFO "- port_id = %u\n", bpdu->port_id && 0xFF);
	//printk(KERN_INFO "- topology_change = %i\n", (bpdu->flags >> 8) & 0xFF);
	//printk(KERN_INFO "- topology_change_ack = %u\n", bpdu->flags & 0xFF);
	//printk(KERN_INFO "- message_age = %u\n", bpdu->message_age);
	//printk(KERN_INFO "- max_age = %u\n", bpdu->max_age);
	//printk(KERN_INFO "- hello_time = %u\n", bpdu->hello_time);	
	//printk(KERN_INFO "- forward_delay = %u\n\n", bpdu->forward_delay);	
}

void received_config_bpdu(struct net_switch_port* p, struct sw_stp_bpdu* config)
{
	struct net_switch* sw = p->sw;
	int was_root = is_root_bridge(p->sw);
	
	//printk(KERN_INFO "Port %s: rcvd config bpdu\n", p->dev->name);
	//print_bpdu(config);
	
	//if(p->stp_vars.state == STP_STATE_DISABLED)
	if(is_disabled_port(p))
		return;
	
	if(supersedes_port_info(p, config))
	{
		//printk(KERN_INFO "Port %d: I have worse BPDU\n", p->stp_vars.port_id & 0xFF);
		
		record_config_information(p, config);
		configuration_update(sw);
		port_state_selection(sw);
		
		if(!is_root_bridge(sw) && was_root)
		{
			//printk(KERN_INFO "Bridge is not root\n");
			del_timer(&sw->stp_vars.hello_timer);
			
			if(sw->stp_vars.topology_change_detected)
			{
				//printk(KERN_INFO "topology_change_detected\n");
				del_timer(&sw->stp_vars.topology_change_timer);
				transmit_tcn(sw);
				mod_timer(&sw->stp_vars.tcn_timer, jiffies + sw->stp_vars.bridge_hello_time);
			}
		} /*end if (!is_root_bridge && was_root)*/
		
		if(p == sw->stp_vars.root_port)
		{
			/*
			printk(KERN_INFO "Port %d(root)[(STATE = %d)]\n", 
			       p->stp_vars.port_id & 0xFF, 
			       p->stp_vars.state
			      );
			*/
			
			record_config_timeout_values(sw, config);
			config_bpdu_generation(sw);
			
			/* if config->topology_change_acknowledge. */
			if(config->flags & 0x01)
				topology_change_acknowledged(sw);
		} /* end if is root port */
		
	} /* If received information is worse the info held by p */
	else if(is_designated_port(p))
	{
		/*
		printk(KERN_INFO "Port %d (designated)[]: I have better BPDU -> send reply\n", p->stp_vars.port_id & 0xFF);
		
		printk(KERN_INFO "- to : ");
		print_bridge_id(&config->bridge_id);
		printk(KERN_INFO "- port_id = %u\n\n", config->port_id);
		*/
		reply(p);
	}
}

static inline void acknowledge_topology_change(struct net_switch_port* p)
{
	p->stp_vars.topology_change_acknowledge = 1;
	
	transmit_config(p);
}

void received_tcn_bpdu(struct net_switch_port* p)
{
	//if(p->stp_vars.state !=	STP_STATE_DISABLED)
	if(!is_disabled_port(p))
	{	
		//printk(KERN_INFO "Port %s: rcvd tcn bpdu\n", p->dev->name);

		if(is_designated_port(p))
		{
			topology_change_detection(p->sw);
			acknowledge_topology_change(p);
		}
	}
}

void become_root_bridge(struct net_switch* sw)
{
	sw->stp_vars.max_age = sw->stp_vars.bridge_max_age;
	sw->stp_vars.hello_time = sw->stp_vars.bridge_hello_time;
	sw->stp_vars.forward_delay = sw->stp_vars.bridge_forward_delay;
		
	topology_change_detection(sw);
	del_timer(&sw->stp_vars.tcn_timer);
	config_bpdu_generation(sw);
	mod_timer(&sw->stp_vars.hello_timer, jiffies + sw->stp_vars.hello_time);
}

void set_bridge_priority(struct net_switch* sw, u16 priority)
{
	int was_root;
	struct net_switch_port* p;
	
	was_root = is_root_bridge(sw);
	
	list_for_each_entry(p, &sw->ports, lh)
	{
		if(is_designated_port(p))
		{
			//memcpy(&p->stp_vars.designated_birdge, &sw->sw->stp_vars.bridge_id, sizeof(bridge_id));
			p->stp_vars.designated_bridge.prio[0] = (priority >> 8) & 0xFF;
			p->stp_vars.designated_bridge.prio[1] = priority  & 0xFF;
		}
	}
	
	sw->stp_vars.bridge_id.prio[0] = (priority >> 8) & 0xFF;
	sw->stp_vars.bridge_id.prio[1] = priority & 0xFF;
	
	configuration_update(sw);
	port_state_selection(sw);
	
	if(is_root_bridge(sw) && !was_root)
	{
		become_root_bridge(sw);
	}
}


void set_sw_hello_time(struct net_switch* sw, char hello_time) {
  spin_lock_bh(&sw->stp_vars.lock);

  sw->stp_vars.bridge_hello_time = hello_time * HZ;
  if (is_root_bridge(sw)) {
    sw->stp_vars.hello_time = sw->stp_vars.bridge_hello_time;
  }
  
  spin_unlock_bh(&sw->stp_vars.lock);
}

void set_sw_max_age(struct net_switch* sw, char max_age) {
  spin_lock_bh(&sw->stp_vars.lock);

  sw->stp_vars.bridge_max_age = max_age * HZ;
  if (is_root_bridge(sw)) {
    sw->stp_vars.max_age = sw->stp_vars.bridge_max_age;
  }
  
  spin_unlock_bh(&sw->stp_vars.lock);
}

void set_sw_forward_delay(struct net_switch* sw, char  forward_delay) {
  spin_lock_bh(&sw->stp_vars.lock);

  sw->stp_vars.bridge_forward_delay = forward_delay * HZ;
  if (is_root_bridge(sw)) {
    sw->stp_vars.forward_delay = sw->stp_vars.bridge_forward_delay;
  }
  
  spin_unlock_bh(&sw->stp_vars.lock);
}

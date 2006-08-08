#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>

#include "sw_private.h"

/**
*/

int designated_for_some_port(struct net_switch* sw)
{
  struct net_switch_port* port;

  list_for_each_entry(port, &sw->ports, lh) 
  {
    if (memcmp(&sw->stp_vars.bridge_id, &port->stp_vars.designated_bridge, sizeof(bridge_id)) == 0)
    {
      return 1;
    }  
  }

  return 0;
}



void sw_hello_timer_expired(unsigned long arg)
{	
	struct net_switch* sw = (struct net_switch*)arg; 
	
	//printk(KERN_INFO "STP HELLO TIMER EXPIRED\n");
	
	spin_lock_bh(&sw->stp_vars.lock);
	config_bpdu_generation(sw);
	mod_timer(&sw->stp_vars.hello_timer, jiffies + sw->stp_vars.hello_time);
	spin_unlock_bh(&sw->stp_vars.lock);
}

void sw_tcn_timer_expired(unsigned long arg)
{
	//printk(KERN_INFO "STP TOPOLOGY CHANGE NOTIFICATION TIMER EXPIRED\n");

	struct net_switch* sw = (struct net_switch*)arg;
	transmit_tcn(sw);

	mod_timer(&sw->stp_vars.tcn_timer, jiffies + sw->stp_vars.bridge_hello_time);
}


void sw_topology_change_timer_expired(unsigned long arg)
{
	//printk(KERN_INFO "STP TOPOLOGY CHANGE TIMER EXPIRED\n");

	struct net_switch* sw = (struct net_switch*)arg;

	spin_lock_bh(&sw->stp_vars.lock);

	sw->stp_vars.topology_change_detected = 0;
	sw->stp_vars.topology_change = 0;

	spin_unlock_bh(&sw->stp_vars.lock);
}



void sw_fdb_cleanup(unsigned long arg)
{
	//printk(KERN_INFO "STP TOPOLOGY CHANGE TIMER EXPIRED\n");
}



void sw_message_age_timer_expired(unsigned long arg)
{
  //printk(KERN_INFO "Message age timer expired");
  int root;

  struct net_switch_port* sw_port = (struct net_switch_port*) arg;
  struct net_switch* sw = sw_port->sw;
  
  spin_lock_bh(&sw->stp_vars.lock);

  root = is_root_bridge(sw);
  become_designated_port(sw_port);
  configuration_update(sw);
  port_state_selection(sw);

  /* Check if bridge has become root */
  if ( (!root) && (is_root_bridge(sw)) )
  {
    sw->stp_vars.max_age = sw->stp_vars.bridge_max_age;
    sw->stp_vars.hello_time = sw->stp_vars.bridge_hello_time;
    sw->stp_vars.forward_delay = sw->stp_vars.bridge_forward_delay;

    topology_change_detection(sw);

    /*do NOT use del_timer_sync while holding lock (posible DEAD LOCK)*/
    del_timer(&sw->stp_vars.tcn_timer);

    config_bpdu_generation(sw);

    mod_timer(&sw->stp_vars.hello_timer, jiffies + sw->stp_vars.hello_time);
  }
  
  spin_unlock_bh(&sw->stp_vars.lock);
}



void sw_forward_delay_timer_expired(unsigned long arg)
{
  printk(KERN_INFO "Delay timer expired");

  struct net_switch_port* sw_port = (struct net_switch_port*)arg;
  struct net_switch* sw = sw_port->sw;

  spin_lock_bh(&sw->stp_vars.lock);

  if (sw_port->stp_vars.state == STP_STATE_LISTENING) 
  {
    //sw_port->stp_vars.state = STP_STATE_LEARNING;
    set_port_state(sw_port, STP_STATE_LEARNING);
    mod_timer(&sw_port->stp_vars.forward_delay_timer, jiffies + sw->stp_vars.forward_delay);
    printk(KERN_INFO "Port %d: entered STP_STATE_LEARNING\n", sw_port->stp_vars.port_id & 0xFF);
  }
  else if (sw_port->stp_vars.state == STP_STATE_LEARNING)
  {
    //sw_port->stp_vars.state = STP_STATE_FORWARDING;
    set_port_state(sw_port, STP_STATE_FORWARDING);
    if (designated_for_some_port(sw)) 
    {
      if (sw_port->stp_vars.change_detection_enabled){
	topology_change_detection(sw);
      }
    }
    printk(KERN_INFO "Port %d: entered STP_STATE_FORWARDING\n", sw_port->stp_vars.port_id & 0xFF);
  }

  spin_unlock_bh(&sw->stp_vars.lock);  
}


	
void sw_hold_timer_expired(unsigned long arg)
{
  //printk(KERN_INFO "HOLD timer expired \n");

  struct net_switch_port* sw_port = (struct net_switch_port*)arg;
  
  spin_lock_bh(&sw_port->sw->stp_vars.lock);
  if (sw_port->stp_vars.config_pending == 1) 
  {
    transmit_config(sw_port);
  }
  spin_unlock_bh(&sw_port->sw->stp_vars.lock);

}



void sw_stp_timers_init(struct net_switch* sw) 
{
	sw_timer_init(&sw->stp_vars.hello_timer, sw_hello_timer_expired, (unsigned long) sw);

	sw_timer_init(&sw->stp_vars.tcn_timer, sw_tcn_timer_expired, (unsigned long) sw);

	sw_timer_init(&sw->stp_vars.topology_change_timer, sw_topology_change_timer_expired, (unsigned long) sw);
	
	sw_timer_init(&sw->stp_vars.gc_timer, sw_fdb_cleanup, (unsigned long) sw);
}



void sw_stp_port_timers_init(struct net_switch_port* p) 
{
	sw_timer_init(&p->stp_vars.message_age_timer, sw_message_age_timer_expired, (unsigned long) p);
	
	sw_timer_init(&p->stp_vars.forward_delay_timer, sw_forward_delay_timer_expired, (unsigned long) p);
	
	sw_timer_init(&p->stp_vars.hold_timer, sw_hold_timer_expired, (unsigned long) p);
}

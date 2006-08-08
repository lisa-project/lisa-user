#include "sw_private.h"


void sw_summary_timer_expired(unsigned long arg)
{
	struct net_switch* sw = (struct net_switch*)arg;
	//printk(KERN_INFO "summary timer expired\n");
	
	spin_lock(&sw->vtp_vars.lock);
	
	vtp_send_summary(sw, INTERRUPT_CONTEXT, 0);
	mod_timer(&sw->vtp_vars.summary_timer, jiffies+VTP_SUMMARY_DELAY);
	
	spin_unlock(&sw->vtp_vars.lock);
}

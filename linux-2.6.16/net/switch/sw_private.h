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

#ifndef _SW_PRIVATE_H
#define _SW_PRIVATE_H

#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/net_switch.h>
#include <linux/timer.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>

#include "sw_private_stp.h"
#include "sw_private_vtp.h"

#define SW_HASH_SIZE_BITS 12
#define SW_HASH_SIZE (1 << SW_HASH_SIZE_BITS)

#define SW_VLAN_ACTIVE		0
#define SW_VLAN_SUSPENDED	1


#define bitoctet(b) ((b)/8)
#define mask(b) ( 1 << (7 - ((b) % 8)) )
/* Set bit b */
#define bitset(bitset, b) ( (bitset)[bitoctet(b)]  |= mask(b) ); printk(KERN_INFO "ADD VLAN for interface %d\n", b);
/* Clear bit b */
#define bitclear(bitset, b) ( (bitset)[bitoctet(b)] &= ~mask(b) ); printk(KERN_INFO "DEL VLAN for interface %d\n", b);
/* Return 0 if bit no set, >0 otherwise */
#define bittest(bitset, b) ( (bitset)[bitoctet(b)] & mask(b) ) 

/* Hash bucket */
struct net_switch_bucket {
	/*
		List of fdb_entries
	*/
	struct list_head entries;

	/* To avoid adding a fdb_entry twice we protect each bucket
	   with a spinlock. Since each bucket has its own lock, this
	   doesn't lead to a bottleneck.
	 */
	spinlock_t lock;
};

struct net_switch_vdb_entry {
	char *name;
	u8 status;
	struct list_head trunk_ports;
	struct list_head non_trunk_ports;
	struct rcu_head rcu;
	
	u16 vlan;
	struct net_switch* sw;
};

struct net_switch_port {
	/* Linking with other ports in a list */
	struct list_head lh;

	/* Physical device associated with this port */
	struct net_device *dev;

	/* Pointer to the switch owning this port */
	struct net_switch *sw;

	unsigned int flags;
	int vlan;

	/* Bitmap of forbidden vlans for trunk ports.
	   512 * 8 bits = 4096 bits => 4096 vlans
	 */
	unsigned char *forbidden_vlans;

	/* Port description */
	char desc[SW_MAX_PORT_DESC + 1];

	/* Physical configuration settings */
	int speed;
	int duplex;

	/* STP */
	struct port_stp_vars stp_vars;

	/* VTP */
	struct sw_port_vtp_vars vtp_vars;
};

struct net_switch_vif_priv {
	struct list_head lh;
	struct net_device_stats stats;
	struct net_switch_port bogo_port;
};

/* Hashing constant for the vlan virtual interfaces hash */
#define SW_VIF_HASH_SIZE 97

struct net_switch {
	/* List of all ports in the switch */
	struct list_head ports;

	/* Switch forwarding database (hashtable) */
	struct net_switch_bucket fdb[SW_HASH_SIZE];

	/* Vlan database */
	struct net_switch_vdb_entry * volatile vdb[SW_MAX_VLAN + 1];

	/* Forwarding database entry aging time */
	atomic_t fdb_age_time;
	
	/* Vlan virtual interfaces */
	struct list_head vif[SW_VIF_HASH_SIZE];
	
	
	/* Cache of forwarding database entries */
	kmem_cache_t *fdb_cache;
	
	/* Cache of link structures */
	kmem_cache_t *vdb_cache;

	/* Template for virtual interfaces mac */
	unsigned char vif_mac[ETH_ALEN];

	/* STP */
	struct bridge_stp_vars stp_vars;

	/* VTP */
	struct sw_vtp_vars vtp_vars;
};


struct net_switch_vdb_link {
	struct list_head lh;
	struct net_switch_port *port;
};

#define sw_disable_port_rcu(port) do {\
	sw_disable_port(port);\
	synchronize_sched();\
} while(0)

#define sw_enable_port_rcu(port) do {\
	sw_enable_port(port);\
	synchronize_sched();\
} while(0)

#define sw_set_port_flag(port,flag) ((port)->flags |= (flag))
#define sw_res_port_flag(port,flag) ((port)->flags &= ~(flag))

#define sw_set_port_flag_rcu(port, flag) do {\
	sw_set_port_flag(port, flag);\
	synchronize_sched();\
} while(0)

#define sw_res_port_flag_rcu(port, flag) do {\
	sw_res_port_flag(port, flag);\
	synchronize_sched();\
} while(0)

/* Hash Entry */
struct net_switch_fdb_entry {
	struct list_head lh;
	unsigned char mac[ETH_ALEN];
	int is_static;
	int vlan;
	struct net_switch_port *port;
	struct net_switch_bucket *bucket;
	struct timer_list age_timer;
	struct rcu_head rcu;
	unsigned long stamp;
};

struct skb_extra {
	int vlan;
	int has_vlan_tag;
};

#define sw_port_forbidden_vlan(port, vlan) sw_forbidden_vlan((port)->forbidden_vlans, vlan)

extern struct net_switch sw;

static __inline__ int sw_mac_hash(const unsigned char *mac) {
	unsigned long x;

	x = mac[0];
	x = (x << 2) ^ mac[1];
	x = (x << 2) ^ mac[2];
	x = (x << 2) ^ mac[3];
	x = (x << 2) ^ mac[4];
	x = (x << 2) ^ mac[5];

	x ^= x >> 8;

	return x & (SW_HASH_SIZE - 1);
}

static __inline__ int sw_vlan_hash(const int vlan) {
	return vlan % SW_VIF_HASH_SIZE; 
}

/* sw.c */
extern void dump_packet(const struct sk_buff *);
extern void sw_enable_port(struct net_switch_port *);
extern void sw_disable_port(struct net_switch_port *);
extern void sw_device_up(struct net_device *);
extern void sw_device_down(struct net_device *);

/* sw_fdb.c */
extern void sw_fdb_init(struct net_switch *);
extern int fdb_cleanup_port(struct net_switch_port *, int);
extern int fdb_cleanup_vlan(struct net_switch *, int, int);
extern int fdb_cleanup_vlan_irq(struct net_switch *, int, int);
extern int fdb_cleanup_by_type(struct net_switch *, int);
extern int fdb_learn(unsigned char *, struct net_switch_port *, int, int, int);
extern int fdb_del(struct net_switch *, unsigned char *,
		struct net_switch_port *, int, int);
extern int fdb_lookup(struct net_switch_bucket *, unsigned char *,
	int, struct net_switch_fdb_entry **);
extern void sw_fdb_exit(struct net_switch *);

/* sw_vdb.c */
extern int sw_vdb_add_vlan(struct net_switch *, int, char *);
extern int sw_vdb_add_vlan_default(struct net_switch *, int);
extern int sw_vdb_del_vlan(struct net_switch *, int);
extern int sw_vdb_del_vlan_irq(struct net_switch *, int);
extern int sw_vdb_set_vlan_name(struct net_switch *, int, char *);
extern void __init sw_vdb_init(struct net_switch *);
extern void __exit sw_vdb_exit(struct net_switch *);
extern int sw_vdb_add_port(int, struct net_switch_port *);
extern int sw_vdb_del_port(int, struct net_switch_port *);

#define sw_vdb_vlan_exists(sw, vlan) \
	(sw_valid_vlan(vlan) && (sw)->vdb[vlan])

/* sw_proc.c */
extern int init_switch_proc(void);
extern void cleanup_switch_proc(void);

/* sw_ioctl.c */
extern int sw_delif(struct net_device *);
extern int sw_deviceless_ioctl(unsigned int, void __user *);
extern void dump_mem(void *, int);

#define VLAN_TAG_BYTES 4

/* sw_forward.c */
extern void add_vlan_tag(struct sk_buff*, int);
extern void strip_vlan_tag(struct sk_buff*); 
extern int sw_forward(struct net_switch_port *,
	struct sk_buff *, struct skb_extra *);

/* sw_vif.c */
extern struct net_device *sw_vif_find(struct net_switch *, int);
extern int sw_vif_addif(struct net_switch *, int);
extern int sw_vif_delif(struct net_switch *, int);
extern int sw_vif_enable(struct net_switch *, int);
extern int sw_vif_disable(struct net_switch *, int);
extern void sw_vif_cleanup(struct net_switch *);
extern void sw_vif_rx(struct sk_buff *);


/**************************************************************************
*		STP files and functions
***************************************************************************/

/* sw_stp.c */
extern void sw_stp_enable(struct net_switch*);
extern void sw_stp_disable(struct net_switch*);
extern void become_designated_port(struct net_switch_port*);
extern void config_bpdu_generation(struct net_switch*);
extern void transmit_config(struct net_switch_port*);
extern void configuration_update(struct net_switch* );
extern void root_selection(struct net_switch* );
extern void designated_port_selection(struct net_switch* ); 
extern void port_state_selection(struct net_switch* );
extern void topology_change_detection(struct net_switch* );
extern void received_config_bpdu(struct net_switch_port*, struct sw_stp_bpdu*); 
extern void received_tcn_bpdu(struct net_switch_port*);
extern void set_bridge_priority(struct net_switch*, u16);
extern void become_root_bridge(struct net_switch*);
extern void set_sw_forward_delay(struct net_switch*, char);
extern void set_sw_max_age(struct net_switch*, char);
extern void set_sw_hello_time(struct net_switch*, char);

/* sw_stp_if.c */
extern void sw_stp_init_port(struct net_switch_port*);
extern void sw_stp_enable_port(struct net_switch_port*);
extern void sw_stp_disable_port(struct net_switch_port*);
extern void sw_stp_handle_bpdu(struct sk_buff* skb);
extern void set_port_priority(struct net_switch_port*, u8);
extern void sw_detect_port_speed(struct net_switch_port*);
extern void set_port_cost(struct net_switch_port*, u32);

/* sw_stp_bpdu.c */
extern void send_config_bpdu(struct net_switch_port*, struct sw_stp_bpdu*);
extern void sw_stp_handle_bpdu(struct sk_buff* skb);
extern void send_tcn_bpdu(struct net_switch_port* );

/* sw_stp_timer.c */
extern void sw_stp_timers_init(struct net_switch*);
extern void sw_stp_timers_start(struct net_switch*);
extern void sw_stp_port_timers_init(struct net_switch_port*);
extern void sw_stp_port_timers_start(struct net_switch_port*);

extern void transmit_tcn(struct net_switch* sw);


static inline int is_designated_port(struct net_switch_port* p)
{
	return !memcmp(&p->stp_vars.designated_bridge, &p->sw->stp_vars.bridge_id, sizeof(bridge_id)) && 
		p->stp_vars.designated_port == p->stp_vars.port_id;
}

static inline int is_disabled_port(struct net_switch_port* p)
{
	return (!(p->dev->flags & IFF_UP) && 
		(p->stp_vars.state == STP_STATE_DISABLED));
}

static inline int is_root_bridge(struct net_switch* sw)
{
	return (memcmp(&sw->stp_vars.bridge_id, &sw->stp_vars.designated_root, sizeof(bridge_id)) == 0);
}

static inline int bridge_id_cmp(bridge_id *a, bridge_id *b)
{
	return memcmp(a, b, sizeof(bridge_id));
}

static inline unsigned long topology_chage_time(struct net_switch* sw)
{
  return sw->stp_vars.bridge_max_age + sw->stp_vars.bridge_forward_delay;
}

static inline void set_port_state(struct net_switch_port* p, u8 state)
{
	p->stp_vars.state = state;
}

static inline void set_path_cost(struct net_switch_port* p, u32 path_cost)
{
	p->stp_vars.path_cost = path_cost;
}

static inline void enable_change_detection(struct net_switch_port* p)
{
	p->stp_vars.change_detection_enabled = 1;
}

static inline void disable_change_detection(struct net_switch_port* p)
{
	p->stp_vars.change_detection_enabled = 0;
}

static inline int can_forward(struct net_switch_port* p)
{
	return !is_disabled_port(p) && 
		atomic_read(&p->sw->stp_vars.stp_enabled) && 
		(p->stp_vars.state == STP_STATE_FORWARDING);
}	

static inline int is_disabled_or_blocked_port(struct net_switch_port* p)
{
	return is_disabled_port(p) || 
		(atomic_read(&p->sw->stp_vars.stp_enabled) && 
		(p->stp_vars.state == STP_STATE_BLOCKING));
}

static inline void sw_timer_init(struct timer_list* timer, void (* function) (unsigned long), unsigned long data) 
{
	//printk(KERN_INFO "Timer init\n");
	
	init_timer(timer);
	timer->data = data;
	timer->function = function;
}


/**************************************************************************
*		VTP files and functions
***************************************************************************/

/* sw_vtp.c */
extern void sw_vtp_init(struct net_switch*);
extern int sw_vtp_enable(struct net_switch*, char);
extern void sw_vtp_disable(struct net_switch*);
extern int sw_vtp_set_domain(struct net_switch*, char*);
extern int sw_vtp_set_mode(struct net_switch*, u8);
extern int sw_vtp_set_password(struct net_switch*, char*, unsigned char*);
extern int sw_vtp_set_version(struct net_switch*, char);
extern void sw_vtp_prune_vlans(struct net_switch_port*, u16);
extern int vtp_pruning_enable(struct net_switch* sw);
extern int vtp_pruning_disable(struct net_switch*);
extern void vtp_rcvd_summary(struct net_switch_port*, struct vtp_summary*);
extern void vtp_rcvd_subset(struct net_switch_port*, struct vtp_subset*);
extern void vtp_rcvd_request(struct net_switch_port*, struct vtp_request*);
extern void vtp_rcvd_join(struct net_switch_port*, struct vtp_join*);
extern int vtp_set_timestamp(struct net_switch*, char*);


/* sw_vtp_messages.c */
extern void vtp_send_message(struct net_switch_port*, unsigned char*, int);
extern void vtp_send_summary(struct net_switch*, u8, u16);
extern void vtp_send_subsets(struct net_switch*, u16);
extern void vtp_send_request(struct net_switch*);
extern void vtp_send_join(struct net_switch*);
extern void vtp_handle_message(struct sk_buff*);
extern void destroy_subset_list(struct net_switch*);
extern void destroy_subset(struct vtp_subset*);


/* sw_vtp_timers.c */
void sw_summary_timer_expired(unsigned long arg);

static inline int is_vtp_enabled(struct net_switch* sw)
{
	return atomic_read(&sw->vtp_vars.vtp_enabled);
}

static inline int is_vtp_client(struct net_switch* sw)
{
	return sw->vtp_vars.mode == VTP_MODE_CLIENT;
}

static inline int is_vtp_server(struct net_switch* sw)
{
	return sw->vtp_vars.mode == VTP_MODE_SERVER;
}

static inline int is_vtp_transparent(struct net_switch* sw)
{
	return sw->vtp_vars.mode == VTP_MODE_TRANSPARENT;
}



#endif

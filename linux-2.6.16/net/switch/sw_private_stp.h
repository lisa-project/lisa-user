#include <asm/atomic.h>

#ifndef _SW_PRIVATE_RSTP_H
#define _SW_PRIVATE_RSTP_H

#define STP_STATE_DISABLED	0
#define STP_STATE_LISTENING	1
#define STP_STATE_LEARNING	2
#define STP_STATE_FORWARDING	3
#define STP_STATE_BLOCKING	4

#define DEFAULT_AGEING_TIME	300

#define DEFAULT_HELLO_TIME	2
#define	MIN_HELLO_TIME		1
#define MAX_HELLO_TIME		2

#define DEFAULT_MAX_AGE		20
#define MIN_MAX_AGE		6
#define MAX_MAX_AGE		40

#define DEFAULT_FORWARD_DELAY	15
#define MIN_FORWARD_DELAY	4
#define MAX_FORWARD_DELAY	30

#define DEFAULT_TX_HOLD_COUNT	6
#define MIN_TX_HOLD_COUNT	1
#define MAX_TX_HOLD_COUNT	10

#define DEFAULT_BRIDGE_PRIORITY	32768
#define DEFAULT_PORT_PRIORITY	128

#define BPDU_TYPE_CONFIG	0
#define BPDU_TYPE_TCN		0x80

#define MESSAGE_AGE_INCREMENT	(1*HZ)

#define HOLD_TIME		(1*HZ)

#define PORT_BITS		8

#define SPEED_10_COST		100
#define SPEED_100_COST		19
#define SPEED_1000_COST		4
#define SPEED_10000_COST	2


typedef struct rstp_priority_vector 	priority_vector;
typedef struct stp_timers		stp_timers;
typedef struct rstp_timers		rstp_timers;
typedef __u16				port_id;
typedef struct identifier		bridge_id;

struct net_switch_port;

struct identifier
{
	unsigned char	prio[2];
	unsigned char	addr[6];
};

struct sw_stp_bpdu
{
	bridge_id	root_id;
	int		root_path_cost;
	bridge_id	bridge_id;
	port_id		port_id;
	unsigned char	flags; 
	int		message_age;
	int		max_age;
	int		hello_time;
	int		forward_delay;
};

struct stp_priority_vector
{
	bridge_id	root_bridge_id;
	u32		root_path_cost;	
	bridge_id	designated_bridge_id;
	port_id		designated_port_id;
	port_id		bridge_port_id;
};


struct bridge_stp_vars
{
	spinlock_t			lock;
        atomic_t                        stp_enabled;  
	bridge_id			designated_root;
	bridge_id			bridge_id;
	u32				root_path_cost;
	
	unsigned long			max_age;
	unsigned long			hello_time;
	unsigned long			forward_delay;
	
	unsigned long			bridge_max_age;
	unsigned long			ageing_time;
	unsigned long			bridge_hello_time;
	unsigned long			bridge_forward_delay;

	struct net_switch_port*		root_port;
        //unsigned char			stp_enabled;
	unsigned char			topology_change;
	unsigned char			topology_change_detected;

	struct timer_list		hello_timer;
	struct timer_list		tcn_timer;
	struct timer_list		topology_change_timer;
	struct timer_list		gc_timer;
	
	struct kobject			ifobj;
};

struct port_stp_vars
{
	u8				priority;
	u8				state;
	
	//u16				port_no; - nu mai avem nevoie de el din moment ce e in strutura portului informatia
	
	unsigned char			topology_change_acknowledge;
	unsigned char			config_pending;
	unsigned char			change_detection_enabled;
	port_id				port_id;
	port_id				designated_port;
	bridge_id			designated_root;
	bridge_id			designated_bridge;
	u32				path_cost;
	u32				designated_cost;

	struct timer_list		forward_delay_timer;
	struct timer_list		hold_timer;
	struct timer_list		message_age_timer;
	struct kobject			kobj;
	struct work_struct		carrier_check;
	struct rcu_head			rcu;
};


static const unsigned char stp_multicast_mac[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

#endif

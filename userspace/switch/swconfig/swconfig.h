#ifndef _SWCONFIG_H
#define _SWCONFIG_H

#include <unistd.h>
#include <errno.h>
#include <linux/sockios.h>

#include "switch.h"
#include "sw_api.h"


#define SWCONFIG_CTX(sw_ops) ((struct swconfig_context *)(sw_ops))

/*	swconfig switch main structure

struct switch_dev {
	int id;
	char dev_name[IFNAMSIZ];
	const char *name;
	const char *alias;
	int ports;
	int vlans;
	int cpu_port;
	struct switch_attr *ops;
	struct switch_attr *port_ops;
	struct switch_attr *vlan_ops;
	struct switch_dev *next;
	void *priv;
};

*/

struct swconfig_context {
	struct switch_operations sw_ops;
	struct switch_dev *dev;
	/* Hardware Switch id: based on HW sw driver register order */
	int switch_id;
	char *dev_name;		/* switch0 */
	char *name;			/* i.e. rt305x-esw */
	int ports;			/* number of HW sw ports */
	int vlans;			/* maximum number of VLANs supported */
	int cpu_port;		/* port_id of CPU trunking port (eth0 as Linux netdev */
} sw_ctx;

struct swconfig_context sw_ctx;

/* extern struct switch_port; */

#define	SWLIB_UNTAGGED	1
#define	SWLIB_TAGGED	0

/*** util_swlib.c functions ***/

/* activate changes in HW switch */
extern int switch_apply(struct swconfig_context *ctx);

/* reset hw switch */
extern int switch_reset(struct swconfig_context *ctx);

/* enable switch VLAN mode */
extern int switch_enable_vlans(struct swconfig_context *ctx); 
/* disable switch VLAN mode */
extern int switch_disable_vlans(struct swconfig_context *ctx);


/*** VLAN Switch Functions ***/

/* get/set VLAN-> port assignments */
extern int get_vlan_ports(int vlan, struct swconfig_context *ctx, int *count, struct switch_port **ports );
extern int set_vlan_ports(int vlan, struct swconfig_context *ctx, int count, struct switch_port **ports);


/*** Switch Port Functions ***/

/* enable/diable switch port */
extern int port_enable(int id, struct swconfig_context *ctx);
extern int port_disable(int id, struct swconfig_context *ctx);

/* set/get switch port untagged */
extern int port_set_untag(int id, struct swconfig_context *ctx, int untag);
extern int port_get_untag(int id, struct swconfig_context *ctx, int *untag);


/* set/get switch port primary vlan id */
extern int port_set_pvid(int id, sw_ctx *ctx, int vlan);
extern int port_get_pvid(int id, sw_ctx *ctx, int *vlan);

/* set/get switch port doubletagging */
extern int port_set_doubletag(int id, struct swconfig_context *ctx);
extern int port_get_doubletag(int id, struct swconfig_context *ctx);

/* set/get switch port HW group: 1 = lan, 0 = wan */
extern int port_set_hwgroup(int id, struct swconfig_context *ctx, int lan);
extern int port_get_hwgroup(int id, struct swconfig_context *ctx, int lan);

/* get switch port link information */
extern int port_get_link(int id, struct swconfig_context *ctx, char *info);

/* get switch port receive good packet count */
extern int port_get_recv_good(int id, struct swconfig_context *ctx, int *count);
/* get switch port receive bad packet count */
extern int port_get_recv_bad(int id, struct swconfig_context *ctx, int *count);



#endif

/*
 * swconfig SYNTAX: 

switch0: rt305x(rt305x-esw), ports: 7 (cpu @ 6), vlans: 4096
     --switch
        Attribute 1 (int): enable_vlan (VLAN mode (1:enabled))
        Attribute 2 (int): alternate_vlan_disable (Use en_vlan instead of doubletag to disable VLAN mode)
        Attribute 3 (none): apply (Activate changes in the hardware)
        Attribute 4 (none): reset (Reset the switch)
     --vlan
        Attribute 1 (ports): ports (VLAN port mapping)
     --port
        Attribute 1 (int): disable (Port state (1:disabled))
        Attribute 2 (int): doubletag (Double tagging for incoming vlan packets (1:enabled))
        Attribute 3 (int): untag (Untag (1:strip outgoing vlan tag))
        Attribute 4 (int): led (LED mode (0:link, 1:100m, 2:duplex, 3:activity, 4:collision, 5:linkact, 6:duplcoll, 7:10mact, 8:100mact, 10:blink, 12:on))
        Attribute 5 (int): lan (HW port group (0:wan, 1:lan))
        Attribute 6 (int): recv_bad (Receive bad packet counter)
        Attribute 7 (int): recv_good (Receive good packet counter)
        Attribute 8 (int): pvid (Primary VLAN ID)
        Attribute 9 (string): link (Get port link information)

 */

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

/*** Global swconfig Switch Functions ***/

/* activate changes in HW switch */
int switch_apply(struct swconfig_context *ctx);

/* reset hw switch */
int switch_reset(struct swconfig_context *ctx);

/* enable switch VLAN mode */
int switch_enable_vlans(struct swconfig_context *ctx); 
/* disable switch VLAN mode */
int switch_disable_vlans(struct swconfig_context *ctx);


/*** VLAN Switch Functions ***/

/* get VLAN port assignments */
int get_vlan_ports(int vlan, struct swconfig_context *ctx, int *ports);


/*** Switch Port Functions ***/

/* enable/diable switch port */
int port_enable(int id, struct swconfig_context *ctx);
int port_disable(int id, struct swconfig_context *ctx);

/* set/get switch port untagged */
int port_set_untag(int untag, int id, struct swconfig_context *ctx);
int port_get_untag(int id, struct swconfig_context *ctx);

/* set/get switch port doubletagging */
int port_set_doubletag(int id, struct swconfig_context *ctx);
int port_get_doubletag(int id, struct swconfig_context *ctx);

/* set/get switch port HW group: 1 = lan, 0 = wan */
int port_set_hwgroup(int lan, int id, struct swconfig_context *ctx);
int port_get_hwgroup(int lan, int id, struct swconfig_context *ctx);

/* get switch port link information */
int port_get_link(int id, char *info, struct swconfig_context *ctx);

/* get switch port receive good packet count */
int port_get_recv_good(int id, struct swconfig_context *ctx);
/* get switch port receive bad packet count */
int port_get_recv_bad(int id, struct swconfig_context *ctx);



#endif

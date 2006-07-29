#ifndef __CDP_IPC_H
#define __CDP_IPC_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <net/if.h>

/* maximum message size */
#define CDP_IPC_MSGSIZE 4096
/* ipc queue key */
#define CDP_IPC_QUEUE_KEY 0x143

/* message types */
#define CDP_IPC_SHOW_QUERY 0x01			/* show cdp command */
#define CDP_IPC_CONF_QUERY 0x02			/* configuration of cdp parameters */
#define CDP_IPC_ADM_QUERY 0x03			/* administrative query */

/* show cdp command types */
#define CDP_IPC_SHOW_CFG 0x01			/* get global configuration parameters */
#define CDP_IPC_SHOW_NEIGHBORS 0x02		/* show cdp neighbors (with filtering options) */
#define CDP_IPC_SHOW_STATS 0x03			/* show cdp traffic */
#define CDP_IPC_SHOW_INTF 0x04			/* show cdp registered interfaces */

/* cdp global configuration field identifiers */
#define CDP_IPC_CFG_VERSION 0x01
#define CDP_IPC_CFG_HOLDTIME 0x02
#define CDP_IPC_CFG_TIMER 0x03
#define CDP_IPC_CFG_ENABLED 0x04

/* cdp administrative query types */
#define CDP_IPC_IF_ENABLE	0x01		/* Enable cdp on an interface */
#define CDP_IPC_IF_DISABLE	0x02		/* Disable cdp on an interface */
#define CDP_IPC_IF_STATUS	0x03		/* Get cdp status on an interface */

/* ipc message structure */
struct cdp_ipc_message {
	int type;					/* message type (pid of the cli process, or 1 for the cdpd thread) */
	int query_type;				/* query type (show, conf or adm) */
	char buf[CDP_IPC_MSGSIZE];	/* message data */
};

/**
 * show cdp ... command query
 */
struct cdp_show_query {
	unsigned char show_type;
	/* filter by interface or device id for "show cdp neighbors" queries */
	unsigned char interface[IFNAMSIZ];	/* show cdp neighbors eth x */
	unsigned char device_id[64];		/* show cdp entry */
};

struct cdp_conf_query {
	unsigned char field_id;
	unsigned char field_value;
};

struct cdp_adm_query {
	unsigned char type;
	char interface[IFNAMSIZ];
};

struct cdp_response {
	char data[CDP_IPC_MSGSIZE-1];
};

struct cdp_ipc_neighbor {
	char interface[IFNAMSIZ];
	struct cdp_neighbor n;
};

#endif

/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#ifndef __CDP_IPC_H
#define __CDP_IPC_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "cdpd.h"

/* maximum message size */
#define CDP_IPC_BUFSIZE 4096
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
	long type;                 /* message type (pid of the cli process, or 1 for the cdpd thread) */
	int query_type;            /* query type (show, conf or adm) */
	char buf[CDP_IPC_BUFSIZE]; /* message data */
};

/* ipc message size for msgsnd, msgrcv */
#define CDP_IPC_MSGSIZE  (sizeof(struct cdp_ipc_message)-sizeof(long))

/**
 * show cdp ... command query
 */
struct cdp_show_query {
	unsigned char show_type;
	/* filter by interface or device id for "show cdp neighbors" queries */
	char interface[IFNAMSIZ];	/* show cdp neighbors eth x */
	char device_id[64];			/* show cdp entry */
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
	char data[CDP_IPC_BUFSIZE-1];
};

struct cdp_ipc_neighbor {
	char interface[IFNAMSIZ];
	struct cdp_neighbor n;
};

#endif

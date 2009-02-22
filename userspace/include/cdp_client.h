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

#ifndef _CDP_CLIENT_H
#define _CDP_CLIENT_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <mqueue.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

/* Message queues:
 *
 * By convention, the cdpd daemon listens for requests on queue lisa-cdp-0
 * and returns responses to lisa-cdp-pid, where pid is the process id of
 * the requesting process. */
#define CDP_QUEUE_NAME "/lisa-cdp-%d"

#define CDP_MAX_RESPONSE_SIZE 4096

#define CDP_CLIENT_TIMEOUT_S 3  /* client receive timeout in seconds */

/* cdp query types */
#define CDP_SHOW_QUERY 0x01			/* show cdp command */
#define CDP_CONF_QUERY 0x02			/* configuration of cdp parameters */
#define CDP_ADM_QUERY  0x03			/* administrative query */

/* show cdp command types */
#define CDP_SHOW_CFG       0x01	    /* get global configuration parameters */
#define CDP_SHOW_NEIGHBORS 0x02     /* show cdp neighbors (with filtering options) */
#define CDP_SHOW_STATS     0x03	    /* show cdp traffic */
#define CDP_SHOW_INTF      0x04     /* show cdp registered interfaces */

/* cdp global configuration field identifiers */
#define CDP_CFG_VERSION  0x01
#define CDP_CFG_HOLDTIME 0x02
#define CDP_CFG_TIMER    0x03
#define CDP_CFG_ENABLED  0x04

/* cdp administrative query types */
#define CDP_IF_ENABLE	0x01		/* Enable cdp on an interface */
#define CDP_IF_DISABLE	0x02		/* Disable cdp on an interface */
#define CDP_IF_STATUS	0x03		/* Get cdp status on an interface */

/**
 * show cdp ... command query
 */
struct cdp_show {
	unsigned char type;
	/* filter by interface or device id for "show cdp neighbors" queries */
	int if_index;		/* show cdp neighbors eth x */
	char device_id[64];	/* show cdp entry */
};

/**
 * cofiguration command query
 */
struct cdp_conf {
	unsigned char field_id;
	unsigned char field_value;
};

/**
 * cdp adminsitrative query
 */
struct cdp_adm {
	unsigned char type;
	int if_index;
};

/* cdp request structure */
struct cdp_request {
	int   type;       /* message type */
	pid_t pid;        /* pid of the requesting process */
	union {           /* request data */
		struct cdp_show show;
		struct cdp_conf conf;
		struct cdp_adm  adm;
	} query;
};

/* Information about a CDP neighbor */
struct cdp_neighbor_info {
	int if_index;							/* Index of the interface */
	unsigned char ttl;						/* Holdtime (time to live) */
	unsigned char cdp_version;
	char device_id[64];						/* Device ID */
	unsigned char num_addr;					/* Number of decoded addresses */
	unsigned int addr[8];					/* Device addresses */
	char port_id[32];						/* Port ID */			
	unsigned char cap;						/* Capabilities */
	char software_version[255];				/* Software (Cisco IOS version) */
	char platform[32];						/* Hardware platform of the device */
	char vtp_mgmt_domain[32];				/* VTP Management Domain */
	unsigned int oui; 						/* OUI (0x00000c for Cisco) */
	unsigned short protocol_id;				/* Protocol Id (0x0112 for Cluster Management) */
	unsigned char payload[27];				/* the other fields */ 
	unsigned char duplex;					/* Duplex */
	unsigned short native_vlan;				/* Native VLAN */
};

/* Information needed by a cdp session */
struct cdp_session {
	int   enabled;                   /* flag indicating if cdp is enabled in this session */
	mqd_t sq, rq;                    /* send and receive message queues */
	char  sq_name[32], rq_name[32];  /* send and receive message queues names */
	char  *cdp_response;             /* the receive buffer for cdp messages */
	int   max_msg_len;               /* the maximum size of messages on the rx queue */
};

/* Initiates a cdp client session */
int cdp_session_start(struct cdp_session *s);

/* Ends a cdp client session */
void cdp_session_end(struct cdp_session *s);

/* Timed receive for a message from the client queue */
int cdp_session_recv(struct cdp_session *s);

#endif

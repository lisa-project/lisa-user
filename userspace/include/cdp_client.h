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

/* By convention, the cdpd daemon listens for requests on queue lisa-cdp-0
 * and returns responses to lisa-cdp-pid, where pid is the process id of
 * the requesting process. */
#define CDP_QUEUE_NAME "/lisa-cdp-%d"

#define CDP_CLIENT_TIMEOUT 3  		/* client receive timeout in seconds */
#define CDP_MAX_RESPONSE_SIZE 4096  /* maximum response message size */

/* CDP global configuration settings default values */
#define CDP_DFL_VERSION 0x02
#define CDP_DFL_HOLDTIME 0xb4
#define CDP_DFL_TIMER 0x3c

/* Device capabilities */
#define CAP_L3R				0x01	/* layer 3 router */
#define CAP_L2TB			0x02	/* layer 2 transparent bridge */
#define CAP_L2SRB			0x04	/* layer 2 source-route bridge */
#define CAP_L2SW			0x08	/* layer 2 switch (non-spanning tree) */
#define CAP_L3HOST			0x10	/* layer 3 (non routing) host */
#define CAP_IGMP			0x20	/* IGMP capable */
#define CAP_L1				0x40	/* layer 1 repeater */

#define DEVICE_CAPABILITIES {\
	{ CAP_L3R,				"Router" },\
	{ CAP_L2TB,				"Trans-Bridge" },\
	{ CAP_L2SRB,			"Source-Route-Bridge" },\
	{ CAP_L2SW,				"Switch" },\
	{ CAP_L3HOST,			"Host" },\
	{ CAP_IGMP,				"IGMP" },\
	{ CAP_L1,				"Repeater" },\
	{ 0,					NULL },\
}

#define DEVICE_CAPABILITIES_BRIEF {\
	{ CAP_L3R,              "R" },\
	{ CAP_L2TB,             "T" },\
	{ CAP_L2SRB,            "B" },\
	{ CAP_L2SW,             "S" },\
	{ CAP_L3HOST,           "H" },\
	{ CAP_IGMP,             "I" },\
	{ CAP_L1,               "r" },\
	{ 0,                    NULL },\
}


/* cdp command types */
enum {
	CDP_SHOW_NEIGHBORS,
	CDP_SHOW_STATS,
	CDP_SHOW_INTF,
	CDP_IF_ENABLE,
	CDP_IF_DISABLE,
	CDP_IF_STATUS
};

/* cdp request structure */
struct cdp_request {
	int type;			/* message type */
	pid_t pid;			/* pid of the requesting process */
	int if_index;		/* filter by interface index */
	char device_id[64];	/* filter by cdp device id */
};

/* CDP configuration parameters */
struct cdp_configuration {
	unsigned char	enabled;				/* enabled flag */
	unsigned char 	version;				/* cdp version */
	unsigned char 	holdtime;				/* cdp ttl (holdtime) in seconds */
	unsigned char	timer;					/* rate at which packets are sent (in seconds) */
	unsigned int	capabilities;			/* device capabilities */
	char			software_version[255];	/* software version information */
	char			platform[16];			/* platform information */
	unsigned char	duplex;					/* duplex */
};

/* CDP traffic statistics */
struct cdp_traffic_stats {
	unsigned int	v1_in;
	unsigned int	v2_in;
	unsigned int	v1_out;
	unsigned int	v2_out;
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

/* cdp client session */
struct cdp_session {
	mqd_t sq, rq;                    /* send and receive message queues */
	char  sq_name[32], rq_name[32];  /* send and receive message queues names */
	char  *response;             	 /* the receive buffer for cdp messages */
	int   max_msg_len;               /* the maximum size of messages on the rx queue */
};

/* Starts a cdp client session */
struct cdp_session *cdp_session_start(void);

/* Ends a cdp client session */
void cdp_session_end(struct cdp_session *s);

/* Sends a request message to the cdp daemon */
#define cdp_session_send(session, req) \
	({\
	 int r = mq_send(session->sq, (const char *)&req, sizeof(req), 0); \
	 r;\
	 })

/* Timed receive for a message from the client queue */
int cdp_session_recv(struct cdp_session *s);

/* Returns 1 if cdp is enabled on the interface, 0 otherwise */
int cdp_is_enabled(struct cdp_session *sessino, int if_index);

/* Enables/disables cdp on an interface */
int cdp_set_interface(struct cdp_session *session, int if_index, int enabled);

/* Requests the list of cdp neighbors from the cdp daemon */
int cdp_get_neighbors(struct cdp_session *session, int if_index, char *device_id);

/* Returns the list of cdp enabled interfaces */
int cdp_get_interfaces(struct cdp_session *session, int if_index);

/* Returns the cdp traffic statistics */
int cdp_get_stats(struct cdp_session *session, struct cdp_traffic_stats *stats);

/* Detailed print of the neighbors from the cdp session response buffer */
void cdp_print_neighbors_detail(struct cdp_session *session, FILE *out);

/* Prints selective information about the neighbors in the cdp session
 * response buffer
 */
void cdp_print_neighbors_filtered(struct cdp_session *session, FILE *out,
		char proto, char version);

/* Prints brief information about the neighbors in the cdp session
 * response buffer
 */
void cdp_print_neighbors_brief(struct cdp_session *session, FILE *out);

#define CDP_SESSION_OPEN(__ctx, __session) ({\
	if (!SWCLI_CTX(__ctx)->cdp) {\
		__session = cdp_session_start();\
	} else\
		__session = SWCLI_CTX(__ctx)->cdp;\
	__session;\
})

#define CDP_SESSION_CLOSE(__ctx, __session) do {\
	if (__session != SWCLI_CTX(__ctx)->cdp) \
		cdp_session_end(__session);\
} while (0)

#endif

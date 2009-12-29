#ifndef _RSTP_CLIENT_H
#define _RSTP_CLIENT_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <mqueue.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

#include "list.h"
#include "vector.h"

/* The RSTP daemon listens for requests on queue lisa-rstp-0
 * and returns responses to lisa-rstp-pid, where pid is the
 * process id of the requesting process */
#define RSTP_QUEUE_NAME "/lisa-rstp-%d"

#define RSTP_CLIENT_TIMEOUT	3
#define RSTP_MAX_RESPONSE_SIZE	4096

/* Sends a request message to the rstp daemon */
#define rstp_session_send(session, req) \
	({\
	 int r = mq_send(session->sq, (const char *)&req, sizeof(req), 0); \
	 r;\
	 })

#define RSTP_SESSION_OPEN(__ctx, __session) ({\
	if (!SWCLI_CTX(__ctx)->rstp) {\
		__session = rstp_session_start();\
	} else\
		__session = SWCLI_CTX(__ctx)->rstp;\
	__session;\
})

#define RSTP_SESSION_CLOSE(__ctx, __session) do {\
	if (__session != SWCLI_CTX(__ctx)->rstp) \
		rstp_session_end(__session);\
} while (0)

/* Starts an rstp client session */
struct rstp_session *rstp_session_start(void);

/* Ends an rstp client session */
void rstp_session_end(struct rstp_session *s);

/* Timed receive for a message from the client queue */
int rstp_session_recv(struct rstp_session *s);

/* Enables/disables rstp on an interface */
int rstp_set_interface(struct rstp_session *session, int if_index, int enabled);

/* rstp client session */
struct rstp_session {
	mqd_t sq, rq;
	char sq_name[32], rq_name[32];
	char *response;
	int max_msg_len;
};

//TODO: these have to be fixed eventually
struct rstp_configuration {
	unsigned char BEGIN; /* not used */

	unsigned char enabled;

	struct priority_vector BridgePriority;
	struct priority_vector rootPriority;

	struct rstp_times BridgeTimes;
	struct rstp_times rootTimes;

	struct bridge_id BridgeIdentifier;

	unsigned char rootPortId[2];

	unsigned int ForceProtocolVersion;

};

struct rstp_bridge {
	struct priority_vector BridgePriority;
	struct priority_vector rootPriority;

	struct rstp_times BridgeTimes;
	struct rstp_times rootTimes;

	unsigned char rootPortId[2];

	struct bridge_id BridgeIdentifier;
};

struct rstp_interface {
	int if_index;
	int sw_sock_fd;
	unsigned char mac_addr[6];
	struct list_head lh;

	struct rstp_bridge *bridge;

	unsigned int state[10][2];

	/* Circular buffer for BPDUs */
	char buf[256];
	//unsigned int buf_start, buf_end;

	/* Port timers */
	volatile unsigned int helloWhen;
	volatile unsigned int tcWhile;
	volatile unsigned int fdWhile;
	volatile unsigned int rcvdInfoWhile;
	volatile unsigned int rrWhile;
	volatile unsigned int rbWhile;
	volatile unsigned int mdelayWhile;
	volatile unsigned int edgeDelayWhile;

	/* Per-port variables */
	volatile unsigned int ageingTime; /* not used */

	volatile unsigned int agree;

	volatile unsigned int agreed;

	struct priority_vector4 designatedPriority;

	struct rstp_times designatedTimes;

	volatile unsigned int disputed;

	volatile unsigned int fdbFlush;

	volatile unsigned int forward;

	volatile unsigned int forwarding;

	volatile unsigned int allSynced;

	volatile unsigned int reRooted;

	volatile unsigned int infoIs;

	volatile unsigned int learn;

	volatile unsigned int learning;

	volatile unsigned int mcheck;

	struct priority_vector4 msgPriority;

	struct rstp_times msgTimes;

	volatile unsigned int newInfo;

	volatile unsigned int operEdge;

	volatile unsigned int portEnabled; /*not used */

	unsigned char portId[2];

	volatile unsigned long PortPathCost;

	struct priority_vector4 portPriority;

	struct rstp_times portTimes;
	
	volatile unsigned int proposed;
	
	volatile unsigned int proposing;

	sem_t rcvdBPDU;
	sem_t processedBPDU;

	volatile unsigned int rcvdInfo;

	volatile unsigned int rcvdMsg;

	volatile unsigned int rcvdRSTP;

	volatile unsigned int rcvdSTP;

	volatile unsigned int rcvdTc;

	volatile unsigned int rcvdTcAck;

	volatile unsigned int rcvdTcn;

	volatile unsigned int reRoot;

	volatile unsigned int reselect;

	volatile unsigned int role;

	volatile unsigned int selected;

	volatile unsigned int selectedRole;

	volatile unsigned int sendRSTP;

	volatile unsigned int sync;

	volatile unsigned int synced;

	volatile unsigned int tcAck;

	volatile unsigned int tcProp;

	volatile unsigned int tick; /*not used*/

	volatile unsigned int txCount;

	volatile unsigned int updtInfo;
};

struct rstp_request {
	int type;
	pid_t pid;
	int if_index;
	char device_id[64];
};

enum {
	RSTP_IF_ENABLE,
	RSTP_IF_DISABLE
};

#endif

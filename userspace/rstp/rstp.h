#ifndef _RSTP_H
#define _RSTP_H

#define RSTP_DEBUG	1

#ifdef RSTP_DEBUG
#define Dprintf(msg,...)	printf(msg, ##__VA_ARGS__)
#else
#define Dprintf(msg,...)
#endif

#include <semaphore.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>
#include <mqueue.h>

#include "list.h"
#include "rstp_bpdu.h"
#include "rstp_utils.h"
#include "util.h"
#include "debug.h"
#include "rstp_client.h"
#include "if_generic.h"
#include "vector.h"

/* listener functions */
void* listener_loop(void*);

/* frame receving loop */
void* rstp_recv_loop(void*);

/* PRX functions */
int prx_discard(struct rstp_interface *);
int prx_receive(struct rstp_interface *);
void updtBPDUVersion(struct rstp_interface *);

/* PST functions */
int pst_discarding(struct rstp_interface *);
int pst_learning(struct rstp_interface *);
int pst_forwarding(struct rstp_interface *);

void pst_disableLearning(void);
void pst_disableForwarding(void);
void pst_enableLearning(void);
void pst_enableForwarding(void);

/* BDM functions */
void bdm_begin_state(struct rstp_interface *);
void bdm_edge(struct rstp_interface *);
void bdm_not_edge(struct rstp_interface *);

/* PIM functions */
void pim_disabled(struct rstp_interface *);
void pim_aged(struct rstp_interface *);
void pim_update(struct rstp_interface *);
void pim_current(struct rstp_interface *);
void pim_receive(struct rstp_interface *);
void pim_superior_designated(struct rstp_interface *);
void pim_repeated_designated(struct rstp_interface *);
void pim_inferior_designated(struct rstp_interface *);
void pim_not_designated(struct rstp_interface *);
void pim_other(struct rstp_interface *);

unsigned int betterorsameInfo(struct rstp_interface *, unsigned int);
unsigned int rcvInfo(struct rstp_interface *);
void recordProposal(struct rstp_interface *);
void setTcFlags(struct rstp_interface *);
void recordPriority(struct rstp_interface *);
void recordTimes(struct rstp_interface *);
void updtRcvdInfoWhile(struct rstp_interface *);
void recordDispute(struct rstp_interface *);
void recordAgreement(struct rstp_interface *);


/* PPM functions */
void ppm_checking_rstp(struct rstp_interface*);
void ppm_selecting_stp(struct rstp_interface*);
void ppm_sensing(struct rstp_interface*);

/* PRS functions */
void* prs_loop(void*);
void prs_init_bridge(void);
void prs_role_selection(void);

/* PTX functions */
void ptx_transmit_init(struct rstp_interface *);
void ptx_idle(struct rstp_interface *);
void ptx_transmit_periodic(struct rstp_interface *);
void ptx_transmit_config(struct rstp_interface *);
void ptx_transmit_tcn(struct rstp_interface *);
void ptx_transmit_rstp(struct rstp_interface *);
void txConfig(struct rstp_interface *);
void txTcn(struct rstp_interface *);
void txRstp(struct rstp_interface *);

/* TCM functions */
void tcm_inactive(struct rstp_interface *);
void tcm_learning(struct rstp_interface *);
void tcm_detected(struct rstp_interface *);
void tcm_active(struct rstp_interface *);
void tcm_notified_tcn(struct rstp_interface *);
void tcm_notified_tc(struct rstp_interface *);
void tcm_propagating(struct rstp_interface *);
void tcm_acknowledged(struct rstp_interface *);

void newTcWhile(struct rstp_interface *);
void setTcPropTree(struct rstp_interface *);
void setTcPropBridge(struct rstp_interface *);

/* PRT functions */
void prt_init_port(struct rstp_interface *port);
void prt_disable_port(struct rstp_interface *port);
void prt_disabled_port(struct rstp_interface *port);
void prt_root_port(struct rstp_interface *port);
void prt_root_proposed(struct rstp_interface *port);
void prt_root_agreed(struct rstp_interface *port);
void prt_reroot(struct rstp_interface *port);
void prt_root_forward(struct rstp_interface *port);
void prt_root_learn(struct rstp_interface *port);
void prt_rerooted(struct rstp_interface *port);
void prt_designated_port(struct rstp_interface *port);
void prt_designated_propose(struct rstp_interface *port);
void prt_designated_synced(struct rstp_interface *port);
void prt_designated_retired(struct rstp_interface *port);
void prt_designated_forward(struct rstp_interface *port);
void prt_designated_learn(struct rstp_interface *port);
void prt_designated_discard(struct rstp_interface *port);
void prt_alternate_port(struct rstp_interface *port);
void prt_alternate_proposed(struct rstp_interface *port);
void prt_alternate_agreed(struct rstp_interface *port);
void prt_block_port(struct rstp_interface *port);
void prt_backup_port(struct rstp_interface *port);

void setSyncTree(void);
void setReRootTree(void);

#define ETH_P_RSTP 0x0022

void rstpd_register_interface(int if_index);
void rstpd_unregister_interface(int if_index);
void *rstp_ipc_listen(void *);

#define MAX_BPDU_SIZE	36
#define BUFSIZE		4800

#define INFO_DISABLED	0
#define INFO_RECEIVED	1
#define INFO_MINE	2
#define INFO_AGED	3

#define NOT_EXECUTED	0
#define EXECUTED	1

#define FUNC		0
#define EXEC		1

#define FSM_NOT_TRANSITIONED	0
#define FSM_TRANSITIONED	1

#define PRX		0
#define PPM		1
#define BDM		2
#define PTX		3
#define PIM		4
#define PRT		5
#define PST		6
#define TCM		7

extern int (*prx_state_table[])(struct rstp_interface *);
extern void (*ppm_state_table[])(struct rstp_interface *);
extern void (*bdm_state_table[])(struct rstp_interface *);
extern void (*ptx_state_table[])(struct rstp_interface *);
extern void (*pim_state_table[])(struct rstp_interface *);
extern void (*prt_state_table[])(struct rstp_interface *);
extern int (*pst_state_table[])(struct rstp_interface *);
extern void (*tcm_state_table[])(struct rstp_interface *);
extern void (*prs_state_table[])(void);

extern volatile unsigned int prs_func;
extern volatile unsigned int prs_exec;

#endif


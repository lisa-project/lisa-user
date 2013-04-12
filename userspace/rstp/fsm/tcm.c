/* TOPOLOGY CHANGE MACHINE */

#include "rstp.h"
#include "switch.h"

#define TCM_INACTIVE		0
#define TCM_LEARNING		1
#define TCM_DETECTED		2
#define TCM_ACTIVE		3
#define TCM_NOTIFIED_TCN	4
#define TCM_NOTIFIED_TC		5
#define TCM_PROPAGATING		6
#define TCM_ACKNOWLEDGED	7

extern struct list_head registered_interfaces;
extern struct rstp_bridge bridge;

void
newTcWhile(struct rstp_interface *port)
{
	//struct rstp_configuration rstp;

	//shared_get_rstp(&rstp);

	if (!port->tcWhile) {
		if (port->sendRSTP) {
			port->tcWhile = port->designatedTimes.HelloTime + 1;
			port->newInfo = 1;
		} else {
			port->tcWhile =bridge.rootTimes.MaxAge + bridge.rootTimes.ForwardDelay;
		}
	}
}

void
setTcPropTree(struct rstp_interface *port)
{
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		if (port->if_index != entry->if_index)
			entry->tcProp = 1;
	}
}

void
setTcPropBridge(struct rstp_interface *port)
{
}

void (*tcm_state_table[8])(struct rstp_interface *) = {tcm_inactive, tcm_learning, tcm_detected, tcm_active,
	tcm_notified_tcn, tcm_notified_tc, tcm_propagating, tcm_acknowledged};

void
tcm_inactive(struct rstp_interface *port)
{
	if (!port->state[TCM][EXEC]) {
		port->fdbFlush = 1;
		port->tcWhile = 0;
		port->tcAck = 0;
	}

	if (port->learn && !port->fdbFlush) {
		port->state[TCM][FUNC] = TCM_LEARNING;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[TCM][EXEC] = EXECUTED;
	}
}

void
tcm_learning(struct rstp_interface *port)
{
	if (!port->state[TCM][EXEC]) {
		port->rcvdTc = port->rcvdTcn = port->rcvdTcAck = 0;
		port->tcWhile = 0;
		port->tcAck = 0;
		port->rcvdTc = port->tcProp = 0;
	}

	if (((port->role == ROLE_ROOT) || (port->role == ROLE_DESIGNATED)) && port->forward && !port->operEdge) {
		port->state[TCM][FUNC] = TCM_DETECTED;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if ((port->role != ROLE_ROOT) && (port->role != ROLE_DESIGNATED) && !(port->learn || port->learning) &&
			!(port->rcvdTc || port->rcvdTcn || port->rcvdTcAck || port->tcProp)) {
		port->state[TCM][FUNC] = TCM_INACTIVE;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if (port->rcvdTc || port->rcvdTcn || port->rcvdTcAck || port->tcProp) {
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[TCM][EXEC] = EXECUTED;
	}
}

void
tcm_detected(struct rstp_interface *port)
{
	newTcWhile(port);
	setTcPropTree(port);
	port->newInfo = 1;

	port->state[TCM][FUNC] = TCM_ACTIVE;
	port->state[TCM][EXEC] = NOT_EXECUTED;
}

void
tcm_active(struct rstp_interface *port)
{
	if (((port->role != ROLE_ROOT) && (port->role != ROLE_DESIGNATED)) || port->operEdge) {
		port->state[TCM][FUNC] = TCM_LEARNING;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if (port->rcvdTcn) {
		port->state[TCM][FUNC] = TCM_NOTIFIED_TCN;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if (port->rcvdTc) {
		port->state[TCM][FUNC] = TCM_NOTIFIED_TC;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if (port->tcProp && !port->operEdge) {
		port->state[TCM][FUNC] = TCM_PROPAGATING;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} else if (port->rcvdTcAck) {
		port->state[TCM][FUNC] = TCM_ACKNOWLEDGED;
		port->state[TCM][EXEC] = NOT_EXECUTED;
	} 
}

void
tcm_notified_tcn(struct rstp_interface *port)
{
	newTcWhile(port);

	port->state[TCM][FUNC] = TCM_NOTIFIED_TC;
	port->state[TCM][EXEC] = NOT_EXECUTED;
}

void
tcm_notified_tc(struct rstp_interface *port)
{
	port->rcvdTcn = port->rcvdTc = 0;
	if (port->role == ROLE_DESIGNATED)
		port->tcAck = 1;
	setTcPropTree(port);

	port->state[TCM][FUNC] = TCM_ACTIVE;
	port->state[TCM][EXEC] = NOT_EXECUTED;
}

void
tcm_propagating(struct rstp_interface *port)
{
	newTcWhile(port);
	port->fdbFlush = 1;
	port->tcProp = 1;

	port->state[TCM][FUNC] = TCM_ACTIVE;
	port->state[TCM][EXEC] = NOT_EXECUTED;
}

void
tcm_acknowledged(struct rstp_interface *port)
{
	port->tcWhile = 0;
	port->rcvdTcAck = 0;

	port->state[TCM][FUNC] = TCM_ACTIVE;
	port->state[TCM][EXEC] = NOT_EXECUTED;
}

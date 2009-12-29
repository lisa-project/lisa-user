/* BRIDGE DETECTION MACHINE */

#include "swsock.h"
#include "rstp.h"
#include "shared.h"

#define BDM_EDGE	0
#define BDM_NOT_EDGE	1

void (*bdm_state_table[3])(struct rstp_interface *) = {bdm_begin_state, bdm_edge, bdm_not_edge};

void
bdm_begin_state(struct rstp_interface *port)
{
	//TODO: support for edge ports
	bdm_not_edge(port);
}

void
bdm_edge(struct rstp_interface *port)
{
	if (!port->state[BDM][EXEC]) {
		port->operEdge = 1;
	}

	if (!port->operEdge) {
		port->state[BDM][FUNC] = BDM_NOT_EDGE;
		port->state[BDM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[BDM][EXEC] = EXECUTED;
	}
}

void
bdm_not_edge(struct rstp_interface *port)
{
	if (!port->state[BDM][EXEC]) {
		port->operEdge = 0;
	}

	if (!port->edgeDelayWhile && port->sendRSTP && port->proposing && 0 /*AutoEdge*/) {
		port->state[BDM][FUNC] = BDM_EDGE;
		port->state[BDM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[BDM][EXEC] = EXECUTED;
	}
}


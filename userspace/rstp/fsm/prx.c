/* PORT RECEIVE MACHINE */

#include "swsock.h"
#include "rstp.h"

#define PRX_DISCARD 0
#define PRX_RECEIVE 1

int (*prx_state_table[2])(struct rstp_interface*) = {prx_discard, prx_receive};

int
prx_discard(struct rstp_interface *port)
{
	if (!port->state[PRX][EXEC]) {
		port->rcvdRSTP = port->rcvdSTP = 0;
		port->rcvdMsg = 0;
		port->edgeDelayWhile = 3;
	}

	if (!sem_trywait(&port->rcvdBPDU)) {
		port->state[PRX][FUNC] = PRX_RECEIVE;
		port->state[PRX][EXEC] = NOT_EXECUTED;
		return FSM_TRANSITIONED;
	} else {
		port->state[PRX][EXEC] = EXECUTED;
		return FSM_NOT_TRANSITIONED;
	}
}

void
updtBPDUVersion(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	//Dprintf("%02x %02x\n", bpdu->hdr.version, bpdu->hdr.bpdu_type);

	if ((bpdu->hdr.version == 0 || bpdu->hdr.version == 1) &&
		(bpdu->hdr.bpdu_type == BPDU_TCN || bpdu->hdr.bpdu_type == BPDU_CONFIG)) {
		port->rcvdSTP = 1;
		//Dprintf("RCVD STP\n");
	}

	if (bpdu->hdr.bpdu_type == BPDU_RSTP) {
		port->rcvdRSTP = 1;
		//Dprintf("RCVD RSTP\n");
	}
}

int
prx_receive(struct rstp_interface *port)
{
	if (!port->state[PRX][EXEC]) {
		updtBPDUVersion(port);
		port->operEdge = 0;
		port->rcvdMsg = 1;
		port->edgeDelayWhile = 3;
	}

	if (!port->rcvdMsg && !sem_trywait(&port->rcvdBPDU)) {
		port->state[PRX][FUNC] = PRX_RECEIVE;
		port->state[PRX][EXEC] = NOT_EXECUTED;
		sem_post(&port->processedBPDU);
		return FSM_TRANSITIONED;
	} else {
		port->state[PRX][EXEC] = EXECUTED;
		return FSM_NOT_TRANSITIONED;
	}
}

/* PORT INFORMATION MACHINE */

#include "swsock.h"
#include "rstp.h"

#define PIM_DISABLED		0
#define PIM_AGED		1
#define PIM_UPDATE		2
#define PIM_CURRENT		3
#define PIM_RECEIVE		4
#define PIM_SUPERIOR_DESIGNATED	5
#define PIM_REPEATED_DESIGNATED	6
#define PIM_INFERIOR_DESIGNATED	7
#define PIM_NOT_DESIGNATED	8
#define PIM_OTHER		9

#define PIM_SUPERIOR_DESIGNATED_INFO		0
#define PIM_REPEATED_DESIGNATED_INFO		1
#define PIM_INFERIOR_DESIGNATED_INFO		2
#define PIM_INFERIOR_ROOT_ALTERNATE_INFO	3
#define PIM_OTHER_INFO				4

unsigned int
betterorsameInfo(struct rstp_interface *port, unsigned int newInfoIs)
{
	return	(newInfoIs == INFO_RECEIVED && port->infoIs == INFO_RECEIVED &&
		 vec_compare4(port->msgPriority, port->portPriority) <= 0) ||
		(newInfoIs == INFO_MINE && port->infoIs == INFO_MINE &&
		 vec_compare4(port->designatedPriority, port->portPriority) <= 0);
}

unsigned int
rcvInfo(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	if (bpdu->hdr.bpdu_type == BPDU_TCN) {
		port->rcvdTcn = 1;
		return PIM_OTHER_INFO;
	}

	if (bpdu->hdr.bpdu_type == BPDU_CONFIG) {
		bpdu->body.flags = bpdu->body.flags | ROLE_DESIGNATED;
	}

	memcpy(&port->msgPriority, bpdu->body.root_id, sizeof(struct priority_vector4));
	memcpy(&port->msgTimes, bpdu->body.message_age, sizeof(struct rstp_times));

	//Dprintf("flags:%x\n", bpdu->body.flags);
	if ((bpdu->body.flags & PORT_ROLE_MASK) == ROLE_DESIGNATED) {
		//print_vector(&port->msgPriority);
		//print_vector(&port->portPriority);
		if ((vec_compare4(port->msgPriority, port->portPriority) < 0)) {
			return PIM_SUPERIOR_DESIGNATED_INFO;
		}

		if (tim_compare(port->msgTimes, port->portTimes) &&
		vec_compare4(port->msgPriority, port->portPriority) == 0) {
			return PIM_SUPERIOR_DESIGNATED_INFO;
		}

		if ((vec_compare4(port->msgPriority, port->portPriority) == 0) &&
		!tim_compare(port->msgTimes, port->portTimes))
			return PIM_REPEATED_DESIGNATED_INFO;

		if (vec_compare4(port->msgPriority, port->portPriority) > 0)
			return PIM_INFERIOR_DESIGNATED_INFO;
	}

	if ((((bpdu->body.flags & PORT_ROLE_MASK) == ROLE_ROOT) ||
		((bpdu->body.flags & PORT_ROLE_MASK) == ROLE_ALTERNATE_BACKUP)) &&
		vec_compare4(port->msgPriority, port->portPriority) >= 0)
		return PIM_INFERIOR_ROOT_ALTERNATE_INFO;

	return PIM_OTHER_INFO;
}

void
recordProposal(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	if (((bpdu->body.flags & PORT_ROLE_MASK) == ROLE_DESIGNATED) &&
		(bpdu->body.flags & PROPOSAL_BIT))
		port->proposed = 1;
}

void
setTcFlags(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	if (bpdu->hdr.bpdu_type == BPDU_TCN) {
		port->rcvdTcn = 1;
	} else {
		if (bpdu->body.flags & TOPOLOGY_CHANGE_BIT)
			port->rcvdTc = 1;
		if (bpdu->body.flags & TOPOLOGY_CHANGE_ACK_BIT)
			port->rcvdTcAck = 1;
	}
}

void
recordPriority(struct rstp_interface *port)
{
	memcpy(&port->portPriority, &port->msgPriority, sizeof(struct priority_vector4));
}

void
recordTimes(struct rstp_interface *port)
{
	memcpy(&port->portTimes, &port->msgTimes, sizeof(struct rstp_times));
}

void
updtRcvdInfoWhile(struct rstp_interface *port)
{
	if (port->portTimes.MessageAge + 1 <= port->portTimes.MaxAge)
		port->rcvdInfoWhile = 3 * port->portTimes.HelloTime;
	else
		port->rcvdInfoWhile = 0;
}

void
recordDispute(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	if ((bpdu->hdr.bpdu_type == BPDU_RSTP) && 
		(bpdu->body.flags & LEARN_BIT)) {
		port->disputed = 1;
		port->agreed = 0;
	}
}

void
recordAgreement(struct rstp_interface *port)
{
	struct bpdu_t *bpdu = (struct bpdu_t *)port->buf;

	if (bpdu->body.flags & AGREEMENT_BIT) {
		port->agreed = 1;
		port->proposing = 0;
	} else {
		port->agreed = 0;
	}
}

void (*pim_state_table[10])(struct rstp_interface *) = 
	{pim_disabled, pim_aged, pim_update, pim_current,
	pim_receive, pim_superior_designated, pim_repeated_designated,
	pim_inferior_designated, pim_not_designated, pim_other};
unsigned int pim_cur_state;

void
pim_disabled(struct rstp_interface *port)
{
	port->rcvdMsg = 0;
	port->proposing = port->proposed = port->agree = port->agreed = 0;
	port->rcvdInfoWhile = 0;
	port->infoIs = INFO_DISABLED;
	port->reselect = 1;
	port->selected = 0;

	port->state[PIM][FUNC] = PIM_AGED;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_aged(struct rstp_interface *port)
{
	if (!port->state[PIM][EXEC]) {
		port->infoIs = INFO_AGED;
		port->reselect = 1;
		port->selected = 0;
	}

	if (port->selected && port->updtInfo) {
		port->state[PIM][FUNC] = PIM_UPDATE;
		port->state[PIM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PIM][EXEC] = EXECUTED;
	}
}

void
pim_update(struct rstp_interface *port)
{
	if (!port->state[PIM][EXEC]) {
		port->proposing = port->proposed = 0;
		port->agreed = port->agreed && betterorsameInfo(port,INFO_MINE);
		port->synced = port->synced && port->agreed;
		memcpy(&port->portPriority, &port->designatedPriority, sizeof(struct priority_vector4));
		memcpy(&port->portTimes, &port->designatedTimes, sizeof(struct rstp_times));
		port->updtInfo = 0;
		port->infoIs = INFO_MINE;
		port->newInfo = 1;
	}

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_current(struct rstp_interface *port)
{
	if (port->selected && port->updtInfo) {
		port->state[PIM][FUNC] = PIM_UPDATE;
		port->state[PIM][EXEC] = NOT_EXECUTED;
	} else if ((port->infoIs == INFO_RECEIVED) && !port->rcvdInfoWhile &&
			!port->updtInfo && !port->rcvdMsg) {
		port->state[PIM][FUNC] = PIM_AGED;
		port->state[PIM][EXEC] = NOT_EXECUTED;
	} else if (port->rcvdMsg && !port->updtInfo) {
		port->state[PIM][FUNC] = PIM_RECEIVE;
		port->state[PIM][EXEC] = NOT_EXECUTED;
	}
}

void
pim_receive(struct rstp_interface *port)
{
	unsigned int rcvdInfo = rcvInfo(port);

	switch(rcvdInfo) {
		case PIM_SUPERIOR_DESIGNATED_INFO:
			port->state[PIM][FUNC] = PIM_SUPERIOR_DESIGNATED;
			port->state[PIM][EXEC] = NOT_EXECUTED;
			//Dprintf("SUPERIOR_DESIGNATED_INFO\n");
			break;
		case PIM_REPEATED_DESIGNATED_INFO:
			port->state[PIM][FUNC] = PIM_REPEATED_DESIGNATED;
			port->state[PIM][EXEC] = NOT_EXECUTED;
			//Dprintf("REPEATED_DESIGNATED_INFO\n");
			break;
		case PIM_INFERIOR_DESIGNATED_INFO:
			port->state[PIM][FUNC] = PIM_INFERIOR_DESIGNATED;
			port->state[PIM][EXEC] = NOT_EXECUTED;
			//Dprintf("INFERIOR_DESIGNATED_INFO\n");
			break;
		case PIM_INFERIOR_ROOT_ALTERNATE_INFO:
			port->state[PIM][FUNC] = PIM_NOT_DESIGNATED;
			port->state[PIM][EXEC] = NOT_EXECUTED;
			//Dprintf("INFERIOR_ROOT_ALTERNATE_INFO\n");
			break;
		case PIM_OTHER_INFO:
			port->state[PIM][FUNC] = PIM_OTHER;
			port->state[PIM][EXEC] = NOT_EXECUTED;
			//Dprintf("OTHER_INFO\n");
			break;
	}
}

void
pim_superior_designated(struct rstp_interface *port)
{
	port->agreed = port->proposing = 0;	
	recordProposal(port);
	setTcFlags(port);
	port->agree = port->agree && betterorsameInfo(port, INFO_RECEIVED);
	recordPriority(port);
	recordTimes(port);
	updtRcvdInfoWhile(port);
	port->infoIs = INFO_RECEIVED;
	port->reselect = 1;
	port->selected = 0;
	port->rcvdMsg = 0;

	sem_post(&port->processedBPDU); //TODO:better solution? 

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_repeated_designated(struct rstp_interface *port)
{
	recordProposal(port);
	setTcFlags(port);
	updtRcvdInfoWhile(port);
	port->rcvdMsg = 0;

	sem_post(&port->processedBPDU); //TODO:better solution? 

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_inferior_designated(struct rstp_interface *port)
{
	recordDispute(port);
	port->rcvdMsg = 0;

	sem_post(&port->processedBPDU); //TODO:better solution? 

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_not_designated(struct rstp_interface *port)
{
	recordAgreement(port);
	setTcFlags(port);
	port->rcvdMsg = 0;

	sem_post(&port->processedBPDU); //TODO:better solution? 

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}

void
pim_other(struct rstp_interface *port)
{
	port->rcvdMsg = 0;

	sem_post(&port->processedBPDU); //TODO:better solution? 

	port->state[PIM][FUNC] = PIM_CURRENT;
	port->state[PIM][EXEC] = NOT_EXECUTED;
}


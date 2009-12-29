/* PORT STATE TRANSITIONS MACHINE */

#include "rstp.h"

#define PST_DISCARDING	0
#define PST_LEARNING	1
#define PST_FORWARDING	2

//TODO: implement learning and forwarding
void
disableLearning(void)
{
	Dprintf("Learning disabled ");
}

void
disableForwarding(void)
{
	Dprintf("Forwarding disabled ");
}

void
enableLearning(void)
{
	Dprintf("Learning enabled ");
}

void
enableForwarding(void)
{
	Dprintf("Forwarding enabled ");
}

int (*pst_state_table[3])(struct rstp_interface *) = {pst_discarding, pst_learning, pst_forwarding};

int
pst_discarding(struct rstp_interface *port)
{
	if (!port->state[PST][EXEC]) {
		disableLearning();
		Dprintf("on interface %d\n", port->if_index);
		port->learning = 0;
		disableForwarding();
		Dprintf("on interface %d\n", port->if_index);
		port->forwarding = 0;
	}

	if (port->learn) {
		port->state[PST][FUNC] = PST_LEARNING;
		port->state[PST][EXEC] = NOT_EXECUTED;
		return FSM_TRANSITIONED;
	} else {
		port->state[PST][EXEC] = EXECUTED;
		return FSM_NOT_TRANSITIONED;
	}
}

int
pst_learning(struct rstp_interface *port)
{
	if (!port->state[PST][EXEC]) {
		enableLearning();
		Dprintf("on interface %d\n", port->if_index);
		port->learning = 1;
	}

	if (port->forward) {
		port->state[PST][FUNC] = PST_FORWARDING;
		port->state[PST][EXEC] = NOT_EXECUTED;
		return FSM_TRANSITIONED;
	} else if (!port->learn) {
		port->state[PST][FUNC] = PST_DISCARDING;
		port->state[PST][EXEC] = NOT_EXECUTED;
		return FSM_TRANSITIONED;
	} else {
		port->state[PST][EXEC] = EXECUTED;
		return FSM_NOT_TRANSITIONED;
	}
}

int
pst_forwarding(struct rstp_interface *port)
{
	if (!port->state[PST][EXEC]) {
		enableForwarding();
		Dprintf("on interface %d\n", port->if_index);
		port->forwarding = 1;
	}

	if (!port->forward) {
		port->state[PST][FUNC] = PST_DISCARDING;
		port->state[PST][EXEC] = NOT_EXECUTED;
		return FSM_TRANSITIONED;
	} else {
		port->state[PST][EXEC] = EXECUTED;
		return FSM_NOT_TRANSITIONED;
	}
}

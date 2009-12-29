/* PORT PROTOCOL MIGRATION MACHINE */

#include "rstp.h"
#include "shared.h"

#define PPM_CHECKING_RSTP	0
#define PPM_SELECTING_STP	1
#define PPM_SENSING		2

void (*ppm_state_table[3])(struct rstp_interface*) = {ppm_checking_rstp, ppm_selecting_stp, ppm_sensing};

void
ppm_checking_rstp(struct rstp_interface *port)
{
	//struct rstp_configuration rstp;

	if (!port->state[PPM][EXEC]) {
		//shared_get_rstp(&rstp);
		port->mcheck = 0;
		port->sendRSTP = 1;//(rstp.ForceProtocolVersion >= 2); /*(rstpVersion)*/
		port->mdelayWhile = 3; /*Migrate Time*/
		//Dprintf("PPM_CHECKING_RSTP\n");
	}

	if (!port->mdelayWhile) {
		port->state[PPM][FUNC] = PPM_SENSING;
		port->state[PPM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PPM][EXEC] = EXECUTED;
	}
}

void
ppm_selecting_stp(struct rstp_interface *port)
{
	if (!port->state[PPM][EXEC]) {
		port->sendRSTP = 0;
		port->mdelayWhile = 3; /*Migrate Time*/
		//Dprintf("PPM_SELECTING_STP\n");
	}

	if (!port->mdelayWhile || port->mcheck) {
		port->state[PPM][FUNC] = PPM_SENSING;
		port->state[PPM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PPM][EXEC] = EXECUTED;
	}
}

void
ppm_sensing(struct rstp_interface *port)
{
	if (!port->state[PPM][EXEC]) {
		port->rcvdRSTP = port->rcvdSTP = 0;
		//Dprintf("PPM_SENSING\n");
	}

	if (port->sendRSTP && port->rcvdSTP) {
		port->state[PPM][FUNC] = PPM_SELECTING_STP;
		port->state[PPM][EXEC] = NOT_EXECUTED;
	} else if (port->mcheck || (!port->sendRSTP && port->rcvdRSTP)){
		port->state[PPM][FUNC] = PPM_CHECKING_RSTP;
		port->state[PPM][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PPM][EXEC] = EXECUTED;
	}
}

/* PORT ROLE TRANSITIONS MACHINE */

#include "swsock.h"
#include "rstp.h"

#define PRT_INIT_PORT		0
#define PRT_DISABLE_PORT	1
#define PRT_DISABLED_PORT	2
#define PRT_ROOT_PORT		3
#define PRT_ROOT_PROPOSED	4
#define PRT_ROOT_AGREED		5
#define PRT_REROOT		6
#define PRT_ROOT_FORWARD	7
#define PRT_ROOT_LEARN		8
#define PRT_REROOTED		9
#define PRT_DESIGNATED_PORT	10
#define PRT_DESIGNATED_PROPOSE	11
#define PRT_DESIGNATED_SYNCED	12
#define PRT_DESIGNATED_RETIRED	13
#define PRT_DESIGNATED_FORWARD	14
#define PRT_DESIGNATED_LEARN	15
#define PRT_DESIGNATED_DISCARD	16
#define PRT_ALTERNATE_PORT	17
#define PRT_ALTERNATE_PROPOSED	18
#define PRT_ALTERNATE_AGREED	19
#define PRT_BLOCK_PORT		20
#define PRT_BACKUP_PORT		21

extern struct list_head registered_interfaces;

void
setSyncTree(void)
{
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		entry->sync = 1;
}

void
setReRootTree(void)
{
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		entry->reRoot = 1;
}

void (*prt_state_table[22])(struct rstp_interface *) = 
	{
	prt_init_port,
	prt_disable_port,
	prt_disabled_port,
	prt_root_port,
	prt_root_proposed,
	prt_root_agreed,
	prt_reroot,
	prt_root_forward,
	prt_root_learn,
	prt_rerooted,
	prt_designated_port,
	prt_designated_propose,
	prt_designated_synced,
	prt_designated_retired,
	prt_designated_forward,
	prt_designated_learn,
	prt_designated_discard,
	prt_alternate_port,
	prt_alternate_proposed,
	prt_alternate_agreed,
	prt_block_port,
	prt_backup_port
	};

unsigned int
checkGlobalConditions(struct rstp_interface *port)
{
	if (port->role != port->selectedRole) {
		switch(port->selectedRole) {
		case ROLE_DISABLED:
			port->state[PRT][FUNC] = PRT_DISABLE_PORT;
			port->state[PRT][EXEC] = NOT_EXECUTED;
			break;
		case ROLE_ROOT:
			port->state[PRT][FUNC] = PRT_ROOT_PORT;
			port->state[PRT][EXEC] = NOT_EXECUTED;
			break;
		case ROLE_DESIGNATED:
			port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
			port->state[PRT][EXEC] = NOT_EXECUTED;
			break;
		case ROLE_ALTERNATE:
		case ROLE_BACKUP:
			port->state[PRT][FUNC] = PRT_BLOCK_PORT;
			port->state[PRT][EXEC] = NOT_EXECUTED;
			break;
		default:
			return 0;
		}

		return 1;
	}

	return 0;
}

void
prt_init_port(struct rstp_interface *port)
{
	port->role = ROLE_DISABLED;
	port->learn = port->forward = 0;
	port->synced = 0;
	port->sync = port->reRoot = 1;
	port->rrWhile = 15;
	port->fdWhile = 20;
	port->rbWhile = 0;

	port->state[PRT][FUNC] = PRT_DISABLE_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_disable_port(struct rstp_interface *port)
{
	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d DISABLE PORT\n", port->if_index);
		port->role = port->selectedRole;
		port->learn = port->forward = 0;
	} else {
		//if (checkGlobalConditions(port))
			//return;
	}

	if (!port->learning && !port->forwarding && port->selected && !port->updtInfo) {
		port->state[PRT][FUNC] = PRT_DISABLED_PORT;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_disabled_port(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d DISABLED PORT\n", port->if_index);
		port->fdWhile = 20;
		port->synced = 1;
		port->rrWhile = 0;
		port->sync = port->reRoot = 0;
	}

	if (port->selected && !port->updtInfo && ((port->fdWhile != 20) || port->sync
		|| port->reRoot || !port->synced)) {
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_root_port(struct rstp_interface *port)
{
	int cond;

	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d ROOT PORT\n", port->if_index);
		port->role = ROLE_ROOT;
		port->rrWhile = 15;
	} else {
		//if (checkGlobalConditions(port))
			//return;
	}

	cond = port->selected && !port->updtInfo;

	if (port->proposed && !port->agree && cond) {
		port->state[PRT][FUNC] = PRT_ROOT_PROPOSED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (((port->allSynced && !port->agree) || (port->proposed && port->agree)) && cond) {
		port->state[PRT][FUNC] = PRT_ROOT_AGREED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (!port->forward && !port->reRoot && cond) {
		port->state[PRT][FUNC] = PRT_REROOT;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((!port->fdWhile || (port->reRooted && !port->rbWhile)) && 1 && port->learn && !port->forward && cond) {
		port->state[PRT][FUNC] = PRT_ROOT_FORWARD;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((!port->fdWhile || (port->reRooted && !port->rbWhile)) && 1 && !port->learn && cond) {
		port->state[PRT][FUNC] = PRT_ROOT_LEARN;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (port->reRoot && port->forward && cond) {
		port->state[PRT][FUNC] = PRT_REROOTED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((port->rrWhile != 15) && cond) {
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_root_proposed(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	setSyncTree();
	port->proposed = 0;

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_root_agreed(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->proposed = port->sync = 0;
	port->agree = 1;
	port->newInfo = 1;

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_reroot(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	setReRootTree();

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_root_forward(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->fdWhile = 0;
	port->forward = 1;

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_root_learn(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->fdWhile = 15;
	port->learn = 1;

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_rerooted(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->reRoot = 0;

	port->state[PRT][FUNC] = PRT_ROOT_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_port(struct rstp_interface *port)
{
	int cond;

	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d DESIGNATED PORT\n", port->if_index);
		port->role = ROLE_DESIGNATED;
	} else {
		//if (checkGlobalConditions(port))
			//return;
	}

	cond = port->selected && !port->updtInfo;

	if (!port->forward && !port->agreed && !port->proposing && !port->operEdge && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_PROPOSE;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (((!port->learning && !port->forwarding && !port->synced) || 
		(port->agreed && !port->synced) || (port->operEdge && !port->synced) ||
		(port->sync && port->synced)) && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_SYNCED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (!port->rrWhile && port->reRoot && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_RETIRED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((!port->fdWhile || port->agreed || port->operEdge) && 
		(!port->rrWhile || !port->reRoot) && !port->sync &&
		(port->learn && !port->forward) && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_FORWARD;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((!port->fdWhile || port->agreed || port->operEdge) && 
		(!port->rrWhile || !port->reRoot) && !port->sync &&
		!port->learn && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_LEARN;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (((port->sync && !port->synced) || (port->reRoot && port->rrWhile) || port->disputed)
		&& !port->operEdge && (port->learn || port->forward) && cond) {
		port->state[PRT][FUNC] = PRT_DESIGNATED_DISCARD;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_designated_propose(struct rstp_interface *port)
{

	//if (checkGlobalConditions(port))
		//return;

	port->proposing = 1;
	port->edgeDelayWhile = 10; /* ? */
	port->newInfo = 1;

	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_synced(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->rrWhile = 0;
	port->synced = 1;
	port->sync = 0;

	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_retired(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->reRoot = 0;
	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_forward(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->forward = 1;
	port->fdWhile = 0;
	port->agreed = port->sendRSTP;

	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_learn(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->learn = 1;
	port->fdWhile = 15;

	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_designated_discard(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->learn = port->forward = port->disputed = 0;
	port->fdWhile = 15;

	port->state[PRT][FUNC] = PRT_DESIGNATED_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_alternate_port(struct rstp_interface *port)
{
	int cond;

	//if (checkGlobalConditions(port))
		//return;

	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d ALTERNATE PORT\n", port->if_index);
		port->fdWhile = 15;
		port->synced = 1;
		port->rrWhile = 0;
		port->sync = port->reRoot = 0;
	}

	cond = port->selected && !port->updtInfo;

	if (port->proposed && !port->agree && cond) {
		port->state[PRT][FUNC] = PRT_ALTERNATE_PROPOSED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if (((port->allSynced && !port->agree) || (port->proposed && port->agree)) && cond) {
		port->state[PRT][FUNC] = PRT_ALTERNATE_AGREED;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else if ((port->rbWhile != 4) && (port->role == ROLE_ALTERNATE_BACKUP) && cond) {
		port->state[PRT][FUNC] = PRT_BACKUP_PORT;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_alternate_proposed(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	setSyncTree();
	port->proposed = 0;

	port->state[PRT][FUNC] = PRT_ALTERNATE_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_alternate_agreed(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->proposed = 0;
	port->agree = 1;
	port->newInfo = 1;

	port->state[PRT][FUNC] = PRT_ALTERNATE_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}

void
prt_block_port(struct rstp_interface *port)
{
	if (!port->state[PRT][EXEC]) {
		if (port->role != port->selectedRole)
			Dprintf("%d BLOCKED PORT\n", port->if_index);
		port->role = port->selectedRole;
		port->learn = port->forward = 0;
	} else {
		//if (checkGlobalConditions(port))
			//return;
	}

	if (!port->learning && !port->forwarding && port->selected && !port->updtInfo) {
		port->state[PRT][FUNC] = PRT_ALTERNATE_PORT;
		port->state[PRT][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PRT][EXEC] = EXECUTED;
	}
}

void
prt_backup_port(struct rstp_interface *port)
{
	//if (checkGlobalConditions(port))
		//return;

	port->rbWhile = 4;

	port->state[PRT][FUNC] = PRT_ALTERNATE_PORT;
	port->state[PRT][EXEC] = NOT_EXECUTED;
}


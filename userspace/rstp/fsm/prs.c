/* PORT ROLE SELECTION MACHINE */

#include "rstp.h"
#include "shared.h"
#include "vector.h"

#define PRS_INIT_BRIDGE		0
#define PRS_ROLE_SELECTION	1

extern struct list_head registered_interfaces;
extern struct rstp_bridge bridge;

void
updtRoleDisabledTree(void)
{
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		entry->selectedRole = ROLE_DISABLED;
}

void
clearReselectTree(void)
{
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		entry->reselect = 0;
}

void
updtRolesTree(void)
{
	struct priority_vector4 root_priority;
	struct priority_vector4 root_path_priority;
	unsigned char root_port_id[2];
	struct rstp_interface *entry, *tmp;
	//struct rstp_configuration rstp;
	struct rstp_interface *root_port = NULL;

	//shared_get_rstp(&rstp);

	memset(&root_priority, 0, sizeof(root_priority));
	memset(&root_port_id, 0, 2);
	memcpy(&root_priority.root_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id));
	memcpy(&root_priority.designated_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id));

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		if (entry->infoIs == INFO_RECEIVED &&
			memcmp(&entry->portPriority.designated_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id)) != 0) {

			root_path_priority = entry->portPriority;
			*(unsigned long*)root_path_priority.root_path_cost =
				*(unsigned long*)root_path_priority.root_path_cost + 200000; /*fix*/

			if ((memcmp(&root_path_priority, &root_priority, sizeof(struct priority_vector4)) < 0) ||
				(((memcmp(&root_path_priority, &root_priority, sizeof(struct priority_vector4)) == 0) &&
					memcmp(&entry->portId, &root_port_id, 2) < 0))) {
				root_priority = root_path_priority;
				memcpy(&root_port_id, &entry->portId, 2);
				root_port = entry;
				}
		}
	}

	memcpy(&bridge.rootPriority, &root_priority, sizeof(struct priority_vector4));
	memcpy(&bridge.rootPortId, root_port_id, 2);

	if (root_port != NULL) {
		memcpy(&bridge.rootTimes, &root_port->portTimes, sizeof(struct rstp_times));
		bridge.rootTimes.MessageAge += 1;
	} else {
		memcpy(&bridge.rootTimes, &bridge.BridgeTimes, sizeof(struct rstp_times));
	}

	//shared_set_rstp(&rstp);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		memcpy(&entry->designatedPriority.root_bridge_id, 
			&root_priority.root_bridge_id, sizeof(struct bridge_id));
		memcpy(&entry->designatedPriority.root_path_cost, 
			&root_priority.root_path_cost, 4);
		memcpy(&entry->designatedPriority.designated_bridge_id, 
			&bridge.BridgeIdentifier, sizeof(struct bridge_id));
		memcpy(&entry->designatedPriority.designated_port_id, 
			&entry->portId, 2);

		memcpy(&entry->designatedTimes, &bridge.BridgeTimes, sizeof(struct rstp_times));
		entry->designatedTimes.HelloTime = bridge.BridgeTimes.HelloTime;
	}

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		switch(entry->infoIs) {
		case INFO_DISABLED:
			entry->selectedRole = ROLE_DISABLED;
			break;
		case INFO_AGED:
			entry->updtInfo = 1;
			entry->selectedRole = ROLE_DESIGNATED;
			break;
		case INFO_MINE:
			entry->selectedRole = ROLE_DESIGNATED;
			if ((vec_compare4(entry->designatedPriority, entry->portPriority) != 0) ||
				tim_compare(entry->designatedTimes, entry->portTimes))
			entry->updtInfo = 1;
			break;
		case INFO_RECEIVED:
			if (entry == root_port) {
				entry->selectedRole = ROLE_ROOT;
				entry->updtInfo = 0;
			} else if (vec_compare4(entry->designatedPriority, entry->portPriority) >= 0) {
				if (memcmp(&entry->portPriority.designated_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id)) != 0)
					entry->selectedRole = ROLE_ALTERNATE;
				else
					entry->selectedRole = ROLE_BACKUP;
				entry->updtInfo = 0;
			} else {
				entry->selectedRole = ROLE_DESIGNATED;
				entry->updtInfo = 1;
			}
			break;
		default:
			break;
		}
	}

	/*list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		switch(entry->selectedRole) {
		case ROLE_DISABLED:
			Dprintf("%d DISABLED\n", entry->if_index);
			break;
		case ROLE_ROOT:
			Dprintf("%d ROOT\n", entry->if_index);
			break;
		case ROLE_DESIGNATED:
			Dprintf("%d DESIGNATED\n", entry->if_index);
			break;
		case ROLE_ALTERNATE:
			Dprintf("%d ALTERNATE\n", entry->if_index);
			break;
		case ROLE_BACKUP:
			Dprintf("%d BACKUP\n", entry->if_index);
			break;
		default:
			Dprintf("HUH?\n");
			break;
		}
	}*/
}

void
setSelectedTree(void)
{
	int allReselected = 0;
	struct rstp_interface *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		allReselected = allReselected || entry->reselect;

	if (!allReselected)
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
			entry->selected = 1;
}

void (*prs_state_table[2])(void) = {prs_init_bridge, prs_role_selection};

volatile unsigned int prs_func;
volatile unsigned int prs_exec;

void
prs_init_bridge(void)
{
	updtRoleDisabledTree();

	prs_func = PRS_ROLE_SELECTION;
	prs_exec = NOT_EXECUTED;
}

void
prs_role_selection(void)
{
	int reselect_ports = 0;
	struct rstp_interface *entry, *tmp;

	if (!prs_exec) {
		clearReselectTree();
		updtRolesTree();
		setSelectedTree();
	}

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		reselect_ports = reselect_ports || entry->reselect; 
	}
	if (reselect_ports) {
		prs_func = PRS_ROLE_SELECTION;
		prs_exec = NOT_EXECUTED;
	} else {
		prs_exec = EXECUTED;
	}
}

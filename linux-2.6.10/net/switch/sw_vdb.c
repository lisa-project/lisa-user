#include "sw_private.h"

void sw_vdb_init(struct net_switch *sw) {
	int i;

	memset(&sw->vdb, 0, sizeof(sw->vdb));
}

#include "sw_fdb.h"
#include "sw_debug.h"

#define DEBUG 1

void __init sw_fdb_init(struct net_switch *sw) {
	int i;

	for (i=0; i<SW_HASH_SIZE; i++) {
		INIT_LIST_HEAD(&sw->hash[i].head);
		init_MUTEX(&sw->hash[i].mutex);
	}
	dbg("Initialized hash of %d buckets\n", SW_HASH_SIZE);	
}

/* Walk the fdb and delete all entries referencing a given port.
 */
void fdb_cleanup_port(struct net_switch_port *port) {
}

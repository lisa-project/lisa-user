#include "sw_fdb.h"
#include "sw_debug.h"

#define DEBUG 1

void __init sw_fdb_init(struct net_switch *sw) {
	int i;

	for (i=0; i<SW_HASH_SIZE; i++) {
		INIT_LIST_HEAD(&sw->hash[i].head);
		rwlock_init(&sw->hash[i].mutex);
	}
	dbg("Initialized hash of %d buckets\n", SW_HASH_SIZE);	
}

/* Walk the fdb and delete all entries referencing a given port.
 */
void fdb_cleanup_port(struct net_switch_port *port) {
}

static inline int __fdb_learn(struct net_switch_bucket *bucket,
		unsigned char *mac, struct net_switch_port *port, int vlan,
		struct net_switch_fdb_entry **pentry) {
	struct list_head p;
	struct net_switch_fdb_entry *entry;

	list_for_each(p, &bucket->entries) {
		entry = list_entry(p, struct net_switch_fdb_entry, lh);
		if(entry->port == port && entry->vlan == vlan &&
				!memcmp(entry.mac, mac)) {
			*pentry = entry;
			return 1;
		}
	}
	return 0;
}

static void fdb_learn(unsigned char *mac, struct net_switch_port *port, int vlan) {
	struct net_switch_bucket *bucket = &sw.fdb[sw_mac_hash(mac)];
	struct net_switch_fdb_entry *entry;

	read_lock(&bucket->lock);
	if(__fdb_learn(bucket, mac, port, vlan, &entry)) {
		/* we found a matching entry */
		entry->stamp = jiffies;
		read_unlock(&bucket->lock);
		return;
	}
	read_unlock(&bucket->lock);

	/* No matching entry. This time lock bucket for write, but search for
	   the entry again, because someone might have added it in the meantime
	 */

	write_lock(&bucket->lock);
	if(__fdb_learn(bucket, mac, port, vlan, &entry)) {
		/* we found a matching entry */
		entry->stamp = jiffies;
		write_unlock(&bucket->lock);
		return;
	}

	/* FIXME alocare entry din cache */

	memcpy(entry.mac, mac, 6);
	entry.vlan = vlan;
	entry.port = port;
	entry.stamp = jiffies;
	list_add_tail(&entry->lh, &bucket->entries);
	write_unlock(&bucket->lock);
}

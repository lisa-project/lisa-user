#include "sw_fdb.h"
#include "sw_debug.h"

static kmem_cache_t *sw_fdb_cache;

void __init sw_fdb_init(struct net_switch *sw) {
	int i;

	for (i=0; i<SW_HASH_SIZE; i++) {
		INIT_LIST_HEAD(&sw->fdb[i].entries);
		rwlock_init(&sw->fdb[i].lock);
	}
	sw_fdb_cache = kmem_cache_create("sw_fdb_cache",
		sizeof(struct net_switch_fdb_entry),
		0, SLAB_HWCACHE_ALIGN, NULL, NULL);
		
	dbg("Initialized hash of %d buckets\n", SW_HASH_SIZE);	
}

/* Walk the fdb and delete all entries referencing a given port.
 */
void fdb_cleanup_port(struct net_switch_port *port) {
}

static inline int __fdb_learn(struct net_switch_bucket *bucket,
		unsigned char *mac, struct net_switch_port *port, int vlan,
		struct net_switch_fdb_entry **pentry) {
	struct net_switch_fdb_entry *entry;

	list_for_each_entry(entry, &bucket->entries, lh) {
		if(entry->port == port && entry->vlan == vlan &&
				!memcmp(entry->mac, mac, ETH_ALEN)) {
			*pentry = entry;
			return 1;
		}
	}
	return 0;
}

void fdb_learn(unsigned char *mac, struct net_switch_port *port, int vlan) {
	struct net_switch_bucket *bucket = &port->sw->fdb[sw_mac_hash(mac)];
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

	/*
		we try to alloc an entry from the cache.
		If that fails we return.
	*/
	entry = kmem_cache_alloc(sw_fdb_cache, GFP_ATOMIC);
	if (!entry) {
		write_unlock(&bucket->lock);
		dbg("cache out of memory");
		return;
	}	

	memcpy(entry->mac, mac, ETH_ALEN);
	entry->vlan = vlan;
	entry->port = port;
	entry->stamp = jiffies;
	list_add_tail(&entry->lh, &bucket->entries);
	write_unlock(&bucket->lock);
}

void __exit sw_fdb_exit(struct net_switch *sw) {
	/* FIXME: kmem_cache_free pe entry-urile care
	 inca sunt in hash */
	int i;
	struct net_switch_fdb_entry *entry;
	
	for (i=0; i<SW_HASH_SIZE; i++) {
		list_for_each_entry(entry, &sw->fdb[i].entries, lh) {
			printk(KERN_DEBUG "removing i=%d entry=%p\n", i, entry);
			kmem_cache_free(sw_fdb_cache, entry);
		}
	}
	kmem_cache_destroy(sw_fdb_cache);
}

/*
 *    This file is part of Linux Multilayer Switch.
 *
 *    Linux Multilayer Switch is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Linux Multilayer Switch is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Linux Multilayer Switch; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sw_private.h"
#include "sw_debug.h"

void __init sw_fdb_init(struct net_switch *sw) {
	int i;

	for (i=0; i<SW_HASH_SIZE; i++) {
		INIT_LIST_HEAD(&sw->fdb[i].entries);
		spin_lock_init(&sw->fdb[i].lock);
	}
	sw->fdb_cache = kmem_cache_create("sw_fdb_cache",
		sizeof(struct net_switch_fdb_entry),
		0, SLAB_HWCACHE_ALIGN, NULL, NULL);
		
	dbg("Initialized hash of %d buckets\n", SW_HASH_SIZE);	
}

/* Walk the fdb and delete all entries referencing a given port.
   
   This is (should be) always called from user space, so locking is
   done _with_ softirqs disabled (spin_lock_bh()).

   Writing is done in a transactional manner: first check if we need
   to change a bucket, and then lock the bucket only if we need to
   change it. While holding the lock search for the entry again to
   avoid races.
 */
void fdb_cleanup_port(struct net_switch_port *port) {
    struct net_switch *sw = port->sw;
    struct net_switch_fdb_entry *entry, *tmp;
	struct list_head *entry_lh, *tmp_lh;
	LIST_HEAD(del_list);
    int i;
	
	for (i = 0; i < SW_HASH_SIZE; i++) {
		list_for_each_entry_rcu(entry, &sw->fdb[i].entries, lh) {
			if(entry->port == port)
                break;
		}
        if(&entry->lh == &sw->fdb[i].entries)
            continue;
        /* We found entries; lock for write and delete them */
        spin_lock_bh(&sw->fdb[i].lock);
		list_for_each_safe_rcu(entry_lh, tmp_lh, &sw->fdb[i].entries) {
			entry = list_entry(entry_lh, struct net_switch_fdb_entry, lh);
			if(entry->port == port) {
                list_del_rcu(&entry->lh);
				list_add(&entry->lh, &del_list);
            }
		}
        spin_unlock_bh(&sw->fdb[i].lock);
	}
	synchronize_kernel();
	list_for_each_entry_safe(entry, tmp, &del_list, lh) {
		dbg("About to free fdb entry at 0x%p for port %s\n",
				entry, entry->port->dev->name);
		kmem_cache_free(sw->fdb_cache, entry);
	}
}

/* Walk the fdb and delete all entries referencing a given vlan.
   
   This is (should be) always called from user space, so locking is
   done _with_ softirqs disabled (spin_lock_bh()).

   Writing is done in a transactional manner: first check if we need
   to change a bucket, and then lock the bucket only if we need to
   change it. While holding the lock search for the entry again to
   avoid races.
 */
void fdb_cleanup_vlan(struct net_switch *sw, int vlan) {
    struct net_switch_fdb_entry *entry, *tmp;
	struct list_head *entry_lh, *tmp_lh;
	LIST_HEAD(del_list);
    int i;
	
	for (i = 0; i < SW_HASH_SIZE; i++) {
		list_for_each_entry_rcu(entry, &sw->fdb[i].entries, lh) {
			if(entry->vlan == vlan)
                break;
		}
        if(&entry->lh == &sw->fdb[i].entries)
            continue;
        /* We found entries; lock for write and delete them */
        spin_lock_bh(&sw->fdb[i].lock);
		list_for_each_safe_rcu(entry_lh, tmp_lh, &sw->fdb[i].entries) {
			entry = list_entry(entry_lh, struct net_switch_fdb_entry, lh);
			if(entry->vlan == vlan) {
                list_del_rcu(&entry->lh);
				list_add(&entry->lh, &del_list);
            }
		}
        spin_unlock_bh(&sw->fdb[i].lock);
	}
	synchronize_kernel();
	list_for_each_entry_safe(entry, tmp, &del_list, lh) {
		dbg("About to free fdb entry at 0x%p for port %s\n",
				entry, entry->port->dev->name);
		kmem_cache_free(sw->fdb_cache, entry);
	}
}

static inline int __fdb_learn(struct net_switch_bucket *bucket,
		unsigned char *mac, struct net_switch_port *port, int vlan,
		struct net_switch_fdb_entry **pentry) {
	struct net_switch_fdb_entry *entry;

	list_for_each_entry_rcu(entry, &bucket->entries, lh) {
		if(entry->vlan == vlan &&
				!memcmp(entry->mac, mac, ETH_ALEN)) {
			*pentry = entry;
			return 1;
		}
	}
	return 0;
}

int fdb_lookup(struct net_switch_bucket *bucket, unsigned char *mac,
		int vlan, struct net_switch_fdb_entry **pentry) {
	struct net_switch_fdb_entry *entry;	

	list_for_each_entry_rcu(entry, &bucket->entries, lh) {
		if (!memcmp(entry->mac, mac, ETH_ALEN) && entry->vlan == vlan) {
			*pentry = entry;
			return 1;
		}
	}
	return 0;
}

/* This is always called from softirq context, so we do locking without
   disabling softirqs
 */
void fdb_learn(unsigned char *mac, struct net_switch_port *port, int vlan) {
	struct net_switch_bucket *bucket = &port->sw->fdb[sw_mac_hash(mac)];
	struct net_switch_fdb_entry *entry;

	if(__fdb_learn(bucket, mac, port, vlan, &entry)) {
		/* we found a matching entry */
		entry->port = port; /* FIXME this should be atomic ??? */
		entry->stamp = jiffies;
		return;
	}

	/* No matching entry. This time lock bucket, but search for the entry
	   again, because someone might have added it in the meantime.
	 */
	spin_lock(&bucket->lock);
	if(__fdb_learn(bucket, mac, port, vlan, &entry)) {
		/* we found a matching entry */
		entry->port = port;
		entry->stamp = jiffies;
		spin_unlock(&bucket->lock);
		return;
	}

	/*
		we try to alloc an entry from the cache.
		If that fails we return.
	*/
	entry = kmem_cache_alloc(port->sw->fdb_cache, GFP_ATOMIC);
	if (!entry) {
		spin_unlock(&bucket->lock);
		dbg("cache out of memory");
		return;
	}	

	memcpy(entry->mac, mac, ETH_ALEN);
	entry->vlan = vlan;
	entry->port = port;
	entry->stamp = jiffies;
	/* FIXME smp_wmb() here ?? */
	list_add_tail_rcu(&entry->lh, &bucket->entries);
	spin_unlock(&bucket->lock);
}

#ifdef DEBUG
EXPORT_SYMBOL(fdb_cleanup_port);
EXPORT_SYMBOL(fdb_lookup);
EXPORT_SYMBOL(fdb_learn);
#endif

void __exit sw_fdb_exit(struct net_switch *sw) {
	/* Entries are freed by __sw_delif(), which is called for
       all interfaces before this
     */
	kmem_cache_destroy(sw->fdb_cache);
}

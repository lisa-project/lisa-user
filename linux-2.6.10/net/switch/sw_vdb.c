#include "sw_private.h"
#include "sw_debug.h"

/* Add a new vlan to the vlan database */
int sw_vdb_add_vlan(struct net_switch *sw, int vlan, char *name) {
	struct net_switch_vdb_entry *entry;

    if(vlan < 1 || vlan > 4095)
        return -ENOMEM;
    if(sw->vdb[vlan])
        return -EEXIST;
    if(!(entry = kmalloc(sizeof(struct net_switch_vdb_entry),
					GFP_ATOMIC))) {
        dbg("Out of memory while trying to add vlan %d\n", vlan);
        return -ENOMEM;
    }
	strncpy(entry->name, name, SW_MAX_VLAN_NAME);
    entry->name[SW_MAX_VLAN_NAME - 1] = '\0';
    INIT_LIST_HEAD(&entry->ports);
	rcu_assign_pointer(sw->vdb[vlan], entry);

	return 0;
}

/* Remove a vlan from the vlan database */
int sw_vdb_del_vlan(struct net_switch *sw, int vlan) {
	struct net_switch_vdb_entry *entry;
	struct net_switch_vdb_link *link, *tmp;

	if(vlan < 1 || vlan > 4095)
		return -EINVAL;
	if(!(entry = sw->vdb[vlan]))
		return -ENOENT;
	rcu_assign_pointer(sw->vdb[vlan], NULL);
	synchronize_kernel();
	list_for_each_entry_safe(link, tmp, &entry->ports, lh) {
		kmem_cache_free(sw->vdb_cache, link);
	}
	kfree(entry);

	return 0;
}

/* Rename a vlan */
int sw_vdb_set_vlan_name(struct net_switch *sw, int vlan, char *name) {
	struct net_switch_vdb_entry *entry, *old;

    if(vlan < 1 || vlan > 4095)
        return -EINVAL;
    if(!(old = sw->vdb[vlan]))
        return -ENOENT;
    if(!(entry = kmalloc(sizeof(struct net_switch_vdb_entry),
					GFP_ATOMIC))) {
        dbg("Out of memory while trying to change vlan name%d\n", vlan);
        return -ENOMEM;
    }
	strncpy(entry->name, name, SW_MAX_VLAN_NAME);
    entry->name[SW_MAX_VLAN_NAME - 1] = '\0';
	entry->ports = old->ports;
	rcu_assign_pointer(sw->vdb[vlan], entry);
	synchronize_kernel();
	kfree(old);

	return 0;
}

/* Initialize the vlan database */
void __init sw_vdb_init(struct net_switch *sw) {
	memset(&sw->vdb, 0, sizeof(sw->vdb));
	sw->vdb_cache = kmem_cache_create("sw_vdb_cache",
			sizeof(struct net_switch_vdb_link),
			0, SLAB_HWCACHE_ALIGN, NULL, NULL);
	if(!sw->vdb_cache)
		return;
    sw_vdb_add_vlan(sw, 1, "default");
    sw_vdb_add_vlan(sw, 1002, "fddi-default");
    sw_vdb_add_vlan(sw, 1003, "trcrf-default");
    sw_vdb_add_vlan(sw, 1004, "fddinet-default");
    sw_vdb_add_vlan(sw, 1005, "trbrf-default");
}

/* Destroy the vlan database */
void __exit sw_vdb_exit(struct net_switch *sw) {
	int vlan;

	for(vlan = 1; vlan <= 4095; vlan++)
		sw_vdb_del_vlan(sw, vlan);
	kmem_cache_destroy(sw->vdb_cache);
}

/* Add a port to a vlan */
int sw_vdb_add_port(int vlan, struct net_switch_port *port) {
	struct net_switch *sw;
	struct net_switch_vdb_link *link;

    if(vlan < 1 || vlan > 4095)
        return -EINVAL;
		
	if (!port) 
		return -EINVAL;	
		
	sw = port->sw; 
	
	if(!sw->vdb[vlan])
		return -ENOENT;
	/* The same port cannot be added twice to the same vlan because the only
	   way to add a port to a vlan is by changing the port's configuration.
	   Changing port configuration is mutually exclusive.
	 */
	link = kmem_cache_alloc(sw->vdb_cache, GFP_ATOMIC);
	if(!link) {
		dbg("Out of memory while adding port to vlan\n");
		return -ENOMEM;
	}
	link->port = port;
	smp_wmb();
	list_add_tail_rcu(&link->lh, &sw->vdb[vlan]->ports);
	dbg("vdb: Added port %s to vlan %d\n", port->dev->name, vlan);

	return 0;
}

/* Remove a port from a vlan */
int sw_vdb_del_port(int vlan, struct net_switch_port *port) {
	struct net_switch_vdb_link *link;

    if(vlan < 1 || vlan > 4095)
        return -EINVAL;
		
	if (!port) 
		return -EINVAL;	
		
	if(!port->sw->vdb[vlan])
		return -ENOENT;
		
	list_for_each_entry(link, &port->sw->vdb[vlan]->ports, lh) {
		if(link->port == port) {
			list_del_rcu(&link->lh);
			synchronize_kernel();
			kmem_cache_free(port->sw->vdb_cache, link);
			dbg("vdb: Removed port %s from vlan %d\n", port->dev->name, vlan);
			return 0;
		}
	}
	
	return -ENOENT;
}

#include "sw_private.h"
#include "sw_debug.h"

static void sw_vdb_add_vlan(struct net_switch *sw, int vlan) {
    if(sw->vdb[vlan])
        return;
    if(!(sw->vdb[vlan] = kmalloc(sizeof(struct net_switch_vdb_entry),
					GFP_ATOMIC))) {
        dbg("Out of memory while trying to add vlan %d\n", vlan);
        return;
    }
    sw->vdb[vlan]->name[0] = '\0';
    sw->vdb[vlan]->name[SW_MAX_VLAN_NAME - 1] = '\0';
    INIT_LIST_HEAD(&sw->vdb[vlan]->ports);
}

void sw_vdb_set_vlan_name(struct net_switch *sw, int vlan, char *name) {
    if(vlan < 1 || vlan > 4095)
        return;
    strncpy(sw->vdb[vlan]->name, name, SW_MAX_VLAN_NAME - 1);
}

void sw_vdb_init(struct net_switch *sw) {
	memset(&sw->vdb, 0, sizeof(sw->vdb));
	sw->vdb_cache = kmem_cache_create("sw_vdb_cache",
			sizeof(struct net_switch_vdb_link),
			0, SLAB_HWCACHE_ALIGN, NULL, NULL);
	if(!sw->vdb_cache)
		return;
    sw_vdb_add_vlan(sw, 1);
    sw_vdb_set_vlan_name(sw, 1, "default");
    sw_vdb_set_vlan_name(sw, 1002, "fddi-default");
    sw_vdb_set_vlan_name(sw, 1003, "trcrf-default");
    sw_vdb_set_vlan_name(sw, 1004, "fddinet-default");
    sw_vdb_set_vlan_name(sw, 1005, "trbrf-default");
}

void sw_vdb_add_port(int vlan, struct net_switch_port *port) {
	struct net_switch *sw = port->sw;
	struct net_switch_vdb_link *link;

    if(vlan < 1 || vlan > 4095)
        return;
	if(!sw->vdb[vlan])
		return;
	/* The same port cannot be added twice to the same vlan because the only
	   way to add a port to a vlan is by changing the port's configuration.
	   Changing port configuration is mutually exclusive.
	 */
	link = kmem_cache_alloc(sw->vdb_cache, GFP_ATOMIC);
	if(!link)
		return;
	link->port = port;
	smp_wmb();
	list_add_tail_rcu(&link->lh, &sw->vdb[vlan]->ports);
}

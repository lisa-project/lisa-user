#include "sw_private.h"
#include "sw_debug.h"

static void __sw_vdb_add_vlan(struct net_switch *sw, int vlan) {
    if(sw->vdb[vlan])
        return;
    if(!(sw->vdb[vlan] = kmalloc(sizeof(struct vdb_entry), GFP_ATOMIC))) {
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
    init_MUTEX(&sw->vdb_sema);
    __sw_vdb_add_vlan(sw, 1);
    sw_vdb_set_vlan_name(sw, 1, "default");
}

void sw_vdb_add_vlan(struct net_switch *sw, int vlan) {
    if(vlan < 1 || vlan > 4095)
        return;
    down_interruptible(&sw->vdb_sema);
    __sw_vdb_add_vlan(sw, vlan);
    up(&sw->vdb_sema);
}

void sw_vdb_add_port(int vlan, struct net_switch_port *port) {
}

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

inline void dump_mem(void *m, int len) {
	int j;
	char buf[65];
	unsigned char *mem= m;

	while(len) {
		for(j = 0; j < 32 &&len; j++, len--) {
			sprintf(buf + 2 * j, "%02hx", *mem);
			mem++;
		}
		dbg("bmp: %s\n", buf);
	}
}

/* Set a forbidden vlan mask to allow the default vlans */
static inline void __sw_allow_default_vlans(unsigned char *forbidden_vlans) {
	sw_allow_vlan(forbidden_vlans, 1);
	sw_allow_vlan(forbidden_vlans, 1002);
	sw_allow_vlan(forbidden_vlans, 1003);
	sw_allow_vlan(forbidden_vlans, 1004);
	sw_allow_vlan(forbidden_vlans, 1005);
}

/* Effectively remove a port from all allowed vlans in a bitmap of
   forbidden vlans.
 */
static inline void __sw_remove_from_vlans(struct net_switch_port *port) {
	int n, vlan = 0;
	unsigned char mask, *bmp = port->forbidden_vlans;
	for(n = 0; n < 512; n++, bmp++) {
		for(mask = 1; mask; mask <<= 1, vlan++) {
			if(*bmp & mask)
				continue;
			sw_vdb_del_port(vlan, port);
		}
	}
}

/* Add an interface to the switch. The switch configuration mutex must
   be acquired from outside.
 */
static int sw_addif(struct net_device *dev) {
	struct net_switch_port *port;
	if(rcu_dereference(dev->sw_port) != NULL) {
		/* dev->sw_port shouldn't be changed elsewhere, so
		   we don't necessarily need rcu_dereference here
		 */
		return -EBUSY;
	}
	if((port = kmalloc(sizeof(struct net_switch_port), GFP_KERNEL)) == NULL)
		return -ENOMEM;
	memset(port, 0, sizeof(struct net_switch_port));
	port->dev = dev;
	port->sw = &sw;
    port->vlan = 1; /* By default all ports are in vlan 1 */
	memset(port->forbidden_vlans, 0xff, 512);
	__sw_allow_default_vlans(port->forbidden_vlans);
    sw_vdb_add_port(1, port);
	list_add_tail_rcu(&port->lh, &sw.ports);
	rcu_assign_pointer(dev->sw_port, port);
	dev_hold(dev);
	dev_set_promiscuity(dev, 1);
	dbg("Added device %s to switch\n", dev->name);
	return 0;
}

/* Remove an interface from the switch. Appropriate locks must be held
   from outside to ensure that nobody tries to remove the same interface
   at the same time.
 */
int sw_delif(struct net_device *dev) {
	struct net_switch_port *port;

	if((port = rcu_dereference(dev->sw_port)) == NULL)
		return -EINVAL;

	/* First disable promiscuous mode, so that there be less chances to
	   still receive packets on this port
	 */
	dev_set_promiscuity(dev, -1);
	/* Now let all incoming queue processors know that frames on this port
	   are not handled by the switch anymore.
	 */
	rcu_assign_pointer(dev->sw_port, NULL);
	/* dev->sw_port is now NULL, so no instance of sw_handle_frame() will
	   process incoming packets on this port.

	   However, at the time we changed the pointer there might have been
	   instances of sw_handle_frame() that were processing incoming
	   packets on this port. Frame processing can add entries to the fdb
	   that reference this port, so we have to wait for all running
	   instances to finish.
	 */
	synchronize_kernel();
	/* Now nobody can add references to this port, so we can safely clean
	   up all existing references from the fdb
	 */
	fdb_cleanup_port(port);
	/* Clean up vlan references: if the port was non-trunk, remove it from
	   its single vlan; otherwise use the disallowed vlans bitmap to remove
	   it from all vlans
	 */
	if(port->flags & SW_PFL_TRUNK) {
		__sw_remove_from_vlans(port);
	} else {
		sw_vdb_del_port(port->vlan, port);
	}
	list_del_rcu(&port->lh);
	/* free port memory and release interface */
	kfree(port);
	dev_put(dev);
	dbg("Removed device %s from switch\n", dev->name);
	return 0;
}

/* Effectively add a port to all allowed vlans in a bitmap of
   forbidden vlans.
 */
static inline void __sw_add_to_vlans(struct net_switch_port *port) {
	int n, vlan = 0;
	unsigned char mask, *bmp = port->forbidden_vlans;
	for(n = 0; n < 512; n++, bmp++) {
		for(mask = 1; mask; mask <<= 1, vlan++) {
			if(*bmp & mask)
				continue;
			sw_vdb_add_port(vlan, port);
		}
	}
}

/* Set a port's trunk mode and make appropriate changes to the
   vlan database.
 */
static int sw_set_port_trunk(struct net_switch_port *port, int trunk) {
	if (!port)
		return -EINVAL;
	if(port->flags & SW_PFL_TRUNK) {
		if(trunk)
			return -EINVAL;
		sw_disable_port_rcu(port);
		__sw_remove_from_vlans(port);
		port->flags &= ~SW_PFL_TRUNK;
		fdb_cleanup_port(port);
		sw_vdb_add_port(port->vlan, port);
		sw_enable_port_rcu(port);
	} else {
		if(!trunk)
			return -EINVAL;
		sw_disable_port_rcu(port);
		sw_vdb_del_port(port->vlan, port);
		port->flags |= SW_PFL_TRUNK;
		fdb_cleanup_port(port);
		__sw_add_to_vlans(port);
		sw_enable_port_rcu(port);
	}
	return 0;
}

/* Change a port's bitmap of forbidden vlans and, if necessary,
   make appropriate changes to the vlan database.
 */
static int sw_set_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	unsigned char mask;
	int n, vlan = 0;

	__sw_allow_default_vlans(forbidden_vlans);
	if(port->flags & SW_PFL_TRUNK) {
		for(n = 0; n < 512; n++, old++, new++) {
			for(mask = 1; mask; mask <<= 1, vlan++) {
				if(!((*old ^ *new) & mask))
					continue;
				if(*new & mask)
					sw_vdb_del_port(vlan, port);
				else
					sw_vdb_add_port(vlan, port);
			}
		}
	}
	memcpy(port->forbidden_vlans, forbidden_vlans, 512);
	return 0;
}

/* Update a port's bitmap of forbidden vlans by allowing vlans from a
   given bitmap of forbidden vlans. If necessary, make the appropriate
   changes to the vlan database.
 */
static int sw_add_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char bmp[512];
	unsigned char *p = bmp;
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	int n;

	for(n = 0; n < 512; n++, old++, new++, p++)
		*p = *old & *new;
	return sw_set_port_forbidden_vlans(port, bmp);
}

/* Update a port's bitmap of forbidden vlans by disallowing those vlans
   that are allowed by a given bitmap (of forbidden vlans). If necessary,
   make the appropriate changes to the vlan database.
 */
static int sw_del_port_forbidden_vlans(struct net_switch_port *port,
		unsigned char *forbidden_vlans) {
	unsigned char bmp[512];
	unsigned char *p = bmp;
	unsigned char *new = forbidden_vlans;
	unsigned char *old = port->forbidden_vlans;
	int n;

	for(n = 0; n < 512; n++, old++, new++, p++)
		*p = *old | ~*new;
	return sw_set_port_forbidden_vlans(port, bmp);
}

/* Change a port's non-trunk vlan and make appropriate changes to the vlan
   database if necessary.
 */
static int sw_set_port_vlan(struct net_switch_port *port, int vlan) {
	if(!port)
		return -EINVAL;
	if(port->vlan == vlan)
		return 0;
	if(!(port->flags & SW_PFL_TRUNK)) {
		sw_vdb_del_port(port->vlan, port);
		sw_vdb_add_port(vlan, port);
	}
	port->vlan = vlan;
	return 0;
}

/* Handle "deviceless" ioctls. These ioctls are not specific to a certain
   device; they control the switching engine as a whole.
 */
int sw_deviceless_ioctl(unsigned int cmd, void __user *uarg) {
	struct net_device *dev;
	struct net_switch_ioctl_arg arg;
	unsigned char bitmap[SW_VLAN_BMP_NO];
	int err = -EINVAL;

	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if ((err = copy_from_user(&arg, uarg, sizeof(struct net_switch_ioctl_arg))))
		return -EINVAL;

	if(cmd != SIOCSWCFG)
		return -EINVAL;

	memset(bitmap, 0xFF, SW_VLAN_BMP_NO);

	/* FIXME zona indicata de arg.name trebuie copiata din userspace */
	switch(arg.cmd) {
	case SWCFG_ADDIF:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_addif(dev);
		dev_put(dev);
		break;
		
	case SWCFG_DELIF:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_delif(dev);
		dev_put(dev);
		break;
	
	case SWCFG_ADDVLAN:
		err = sw_vdb_add_vlan(&sw, arg.vlan, arg.name);
		break;
	case SWCFG_DELVLAN:
		err = sw_vdb_del_vlan(&sw, arg.vlan);
		break;
	case SWCFG_RENAMEVLAN:
		err = sw_vdb_set_vlan_name(&sw, arg.vlan, arg.name);
		break;
	case SWCFG_ADDVLANPORT:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		sw_allow_vlan(bitmap, arg.vlan);
		err = sw_add_port_forbidden_vlans(rcu_dereference(dev->sw_port), bitmap);
		break;
	case SWCFG_DELVLANPORT:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		/* use sw_allow_vlan here because sw_del_port_forbidden_vlans
		   negates the mask
		 */
		sw_allow_vlan(bitmap, arg.vlan);	
		err = sw_del_port_forbidden_vlans(rcu_dereference(dev->sw_port), bitmap);
		break;
	case SWCFG_SETTRUNK:
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_set_port_trunk(rcu_dereference(dev->sw_port), arg.vlan);	
		break;
	case SWCFG_SETPORTVLAN:	
		if((dev = dev_get_by_name(arg.name)) == NULL) {
			err = -ENODEV;
			break;
		}
		err = sw_set_port_vlan(rcu_dereference(dev->sw_port), arg.vlan);	
		break;
	}

	return err;
}

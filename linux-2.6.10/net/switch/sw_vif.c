#include "sw_private.h"
#include "sw_debug.h"

struct net_device *sw_vif_find(struct net_switch *sw, int vlan) {
	struct net_switch_vif_priv *priv;
	list_for_each_entry(priv, &sw->vif, lh) {
		if(priv->vlan == vlan)
			return priv->dev;
	}
	return NULL;
}

int sw_vif_addif(struct net_switch *sw, int vlan) {
	char buf[9];
	struct net_device *dev;
	struct net_switch_vif_priv *priv;
	int result;
	
	if(vlan < 1 || vlan > 4095)
		return -EINVAL;
	if(sw_vif_find(sw, vlan))
		return -EEXIST;
	/* We can now safely create the new interface and this is no race
	   because this is called only from ioctl() and ioctls are
	   mutually exclusive (a semaphore in socket ioctl routine)
	 */
	sprintf(buf, "vlan%d", vlan);
	dbg("About to alloc netdev for vlan %d\n", vlan);
	dev = alloc_netdev(sizeof(struct net_switch_vif_priv), buf, ether_setup);
	if(dev == NULL)
		return -EINVAL;
	memcpy(dev->dev_addr, sw->vif_mac, ETH_ALEN);
	dev->dev_addr[ETH_ALEN - 2] ^= vlan / 0x100;
	dev->dev_addr[ETH_ALEN - 1] ^= vlan % 0x100;
	priv = netdev_priv(dev);
	priv->sw = sw;
	priv->dev = dev;
	priv->vlan = vlan;
	list_add_tail(&priv->lh, &sw->vif);
	if ((result = register_netdev(dev))) {
		dbg("vif: error %i registering netdevice %s\n", 
				result, dev->name);
	}
	else {
		dbg("vif: successfully registered netdevice %s\n", dev->name);
	}		
	sw_vdb_add_vlan_default(sw, vlan);
	
	return 0;
}

static void __vif_delif(struct net_device *dev) {
	struct net_switch_vif_priv *priv;

	priv = netdev_priv(dev);
	list_del_rcu(&priv->lh);
	unregister_netdev(dev);
	synchronize_kernel();
	free_netdev(dev);
}

int sw_vif_delif(struct net_switch *sw, int vlan) {
	struct net_device *dev;

	if(vlan < 1 || vlan > 4095)
		return -EINVAL;
	if((dev = sw_vif_find(sw, vlan)) == NULL)
		return -ENOENT;

	__vif_delif(dev);
	return 0;
}

void sw_vif_cleanup(struct net_switch *sw) {
	struct net_switch_vif_priv *priv, *tmp;
	list_for_each_entry_safe(priv, tmp, &sw->vif, lh)
		__vif_delif(priv->dev);
}

#ifndef _UTIL_LINUX_H
#define _UTIL_LINUX_H

#include <unistd.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/net_switch.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>


#include "switch.h"
#include "if_generic.h"
#include "swsock.h"
#include "sw_api.h"
#include "util.h"

/* Check if there is a VLAN in switch */
int has_vlan(int vlan);

/* Check if the switch has a VLAN interface */
int has_vlan_if(int vlan);

#endif

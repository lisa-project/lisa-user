/*
 *    This file is part of LiSA Switch
 *
 *    LiSA Switch is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2 of the
 *    License, or (at your option) any later version.
 *
 *    LiSA Switch is distributed in the hope that it will be
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Switch; if not, write to the Free
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 */

#include "swconfig.h"
#include <swlib.h>


/* activate changes in HW switch */
int switch_apply(sw_ctx *ctx){

}

/* reset hw switch */
int switch_reset(sw_ctx *ctx){

}

/* enable switch VLAN mode */
int switch_enable_vlans(sw_ctx *ctx){

} 

/* disable switch VLAN mode */
int switch_disable_vlans(sw_ctx *ctx){

}


/*** VLAN Switch Functions ***/

/* get VLAN->Port assignments */
int get_vlan_ports(int vlan, sw_ctx *ctx, int *ports){

}


/*** Switch Port Functions ***/

/* enable/disable switch port */
int port_enable(int id, sw_ctx *ctx){

}

int port_disable(int id, sw_ctx *ctx){

}

/* set/get switch port untagged */
int port_set_untag(int untag, int id, sw_ctx *ctx){

}

int port_get_untag(int id, sw_ctx *ctx){

}


/* set/get switch port doubletagging */
int port_set_doubletag(int id, sw_ctx *ctx){

}

int port_get_doubletag(int id, sw_ctx *ctx){

}

/* set/get switch port HW group: 1 = lan, 0 = wan */
int port_set_hwgroup(int lan, int id, sw_ctx *ctx){

}

int port_get_hwgroup(int lan, int id, sw_ctx *ctx){

}

/* get switch port link information */
int port_get_link(int id, char *info, sw_ctx *ctx){

}

/* get switch port receive good packet count */
int port_get_recv_good(int id, sw_ctx *ctx){

}
/* get switch port receive bad packet count */
int port_get_recv_bad(int id, sw_ctx *ctx){

}



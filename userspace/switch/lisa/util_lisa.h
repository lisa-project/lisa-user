/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#ifndef _UTIL_LISA_H
#define _UTIL_LISA_H

#include <linux/net_switch.h>

#define SW_MAX_VLAN 4094

#define SW_VLAN_BMP_NO (SW_MAX_VLAN / 8 + 1)

#define	_SC_PAGE_SIZE		_SC_PAGESIZE

int buf_alloc_swcfgr(struct swcfgreq *swcfgr, int sock_fd);

#endif

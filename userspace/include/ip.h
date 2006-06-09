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

#ifndef __SW_IP_H
#define __SW_IP_H

#include "netlink.h"
#include "list.h"

#define MAX_NAME_LEN 16

extern int change_ip_address(int, char *, char *, int);
extern struct list_head *list_ip_addr(char *, int);

struct ip_addr_entry {
	struct list_head lh;
	char inet[MAX_NAME_LEN];
	char mask[MAX_NAME_LEN];
};

#endif

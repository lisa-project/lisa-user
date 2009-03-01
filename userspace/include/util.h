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
#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>

#define PAGE_SIZE 4096
// FIXME it's better to determine it at compile time
// by using an auxiliary test program and getpagesize() - for further
// details, see man 2 getpagesize

#define MAX_HOSTNAME 32

void daemonize(void);
void print_mac(FILE *out, void *buf, int size, char *(*get_if_name)(int, void*), void *priv);
int buf_alloc_swcfgr(struct swcfgreq *swcfgr, int sock_fd);

#endif

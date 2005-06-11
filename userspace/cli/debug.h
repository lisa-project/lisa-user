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

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DEBUG
#include <stdio.h>
#define dbg(text,par...) do {\
	fprintf(stderr, text, ##par);\
	fflush(stderr);\
} while(0)
#define __dbg_static

static inline void dump_mem(void *m, int len) {
	int j;
	char buf[49];
	unsigned char *mem= m;

	while(len) {
		for(j = 0; j < 16 &&len; j++, len--) {
			sprintf(buf + 3 * j, "%02hx ", *mem);
			mem++;
		}
		dbg("bmp: %s\n", buf);
	}
}
#else
#define dbg(par...)
#define __dbg_static static
#define dump_mem(m, len)
#endif

#endif

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

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/sockios.h>

#include "util_lisa.h"

int buf_alloc_swcfgr(struct swcfgreq *swcfgr, int sock_fd)
{
	void *buf;
	int page_size = sysconf(_SC_PAGE_SIZE);
	int size = page_size;
	int status;

	buf = malloc(size);
	if (buf == NULL) {
		errno = ENOMEM;
		return -1;
	}

	do {
		swcfgr->buf.size = size;
		swcfgr->buf.addr = buf;
		status = ioctl(sock_fd, SIOCSWCFG, swcfgr);
		if (status >= 0)
			return status;

		if (errno != ENOMEM)
			return -1;

		size += page_size;
		buf = realloc(buf, size);
		if (buf == NULL) {
			errno = ENOMEM;
			return -1;
		}
	} while (1);
}

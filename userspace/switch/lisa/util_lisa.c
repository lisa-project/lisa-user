#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/sockios.h>

#include "util.h"

int buf_alloc_swcfgr(struct swcfgreq *swcfgr, int sock_fd)
{
	void *buf;
	int page_size = sysconf(_SC_PAGE_SIZE);
	int size = page_size;
	int status;

	buf = malloc(size);
	if (buf == NULL)
		return -ENOMEM;

	do {
		swcfgr->buf.size = size;
		swcfgr->buf.addr = buf;
		status = ioctl(sock_fd, SIOCSWCFG, swcfgr);
		if (status >= 0)
			return status;

		if (errno != ENOMEM)
			return -errno;

		size += page_size;
		buf = realloc(buf, size);
		if (buf == NULL)
			return -ENOMEM;
	} while (1);
}

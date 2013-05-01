#ifndef _LINUX_H
#define _LINUX_H

#include <unistd.h>
#include <linux/sockios.h>

#include "sw_api.h"

struct linux_context {
	struct switch_operations sw_ops;
};

extern struct linux_context lnx_ctx;

#endif

#ifndef _LISA_H
#define _LISA_H

#include "sw_api.h"

struct lisa_context {

	struct switch_operations sw_ops;

	/* Socket descriptor used for communication with LiSA module. */
	int sock_fd;
};

#endif

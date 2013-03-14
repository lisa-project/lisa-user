#ifndef _LISA_H
#define _LISA_H

#include "sw_api.h"
#include "../command/swcli.h"
#include "../command/config.h"

struct lisa_context {
	struct switch_operations lisa_ops;
	struct cli_context ctx;
};

#endif

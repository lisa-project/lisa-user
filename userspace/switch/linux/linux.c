#include <linux/net_switch.h>
#include <errno.h>

#include "switch.h"
#include "util_linux.h"
#include "linux.h"

/**
 * TODO Create a bridge
 */
static int linux_init(struct switch_operations *sw_ops)
{
	return 0;
}

struct linux_context lnx_ctx = {
	.sw_ops = (struct switch_operations) {
		.backend_init = linux_init
	}
};

#include "cli.h"
#include "swcli.h"

struct menu_node menu_main = {
	/* Root node, .name is used as prompt */
	.name			= "",
	.subtree	= (struct menu_node[]) {
		{ /* #clear */
			.name			= "clear",
			.help			= "Reset functions",
			.mask			= PRIV(2),
			.tokenize = NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) {
			}
		},

		{ /* #configure */
			.name			= "configure"
		},

		NULL_MENU_NODE
	}
};

/* vim: ts=2 shiftwidth=2
 */

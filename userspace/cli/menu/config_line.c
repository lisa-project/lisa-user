#include "swcli.h"
#include "config_line.h"

struct menu_node config_line_main = {
	/* Root node, .name is used as prompt */
	.name			= "config-line",
	.subtree	= (struct menu_node *[]) {
		/* #password */
		& (struct menu_node){
			.name			= "password",
			.help			= "Set a password",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= swcli_tokenize_line_mixed,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #password 0 */
				& (struct menu_node){
					.name			= "0",
					.help			= "Specifies an UNENCRYPTED password will follow",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= swcli_tokenize_line,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #password 0 LINE */
						& (struct menu_node){
							.name			= "LINE",
							.help			= "The UNENCRYPTED (cleartext) line password",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_setpw,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #password LINE */
				& (struct menu_node){
					.name			= "LINE",
					.help			= "The UNENCRYPTED (cleartext) line password",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_setpw,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #exit */
		& (struct menu_node){
			.name			= "exit",
			.help			= "Exit from line configuration mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		/* #end */
		& (struct menu_node){
			.name			= "end",
			.help			= "End line configuration mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_end,
			.subtree	= NULL
		},

		NULL
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/

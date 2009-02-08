#include "cli.h"
#include "swcli_common.h"

int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
int cmd_namevlan(struct cli_context *, int, char **, struct menu_node **);

extern struct menu_node config_vlan;

struct menu_node config_vlan_main = {
	/* Root node, .name is used as prompt */
	.name			= "config-vlan",
	.subtree	= (struct menu_node *[]) {
		/* #exit */
		& (struct menu_node){
			.name			= "exit",
			.help			= "Apply changes, bump revision number, and exit mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		/* #name */
		& (struct menu_node){
			.name			= "name",
			.help			= "Ascii name of the VLAN",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= swcli_tokenize_word,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #name WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "The ascii name for the VLAN",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_namevlan,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #no */
		& (struct menu_node){
			.name			= "no",
			.help			= "Negate a command or set its defaults",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #no name */
				& (struct menu_node){
					.name			= "name",
					.help			= "Ascii name of the VLAN",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_namevlan,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #vlan */
		& config_vlan,

		NULL
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/

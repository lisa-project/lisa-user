#include "cli.h"
#include "swcli_common.h"

/*                         show
 *                           |
 *        +------------------+------------------+
 *        |                                     |
 *       mac                                    |
 *        |                             mac-address-table
 *  address-table                               |
 *        |                                     |
 *        +------------------+------------------+
 *                           |
 *        +------------------+------------------+
 *        |                  |                  |
 *     dynamic             static               |
 *        |                  |                  |
 *        +------------------+------------------+
 *                           |
 *        +------------------+------------------+
 *        |                                     |
 *     address                                  |
 *        |                                     |
 *      H.H.H                                   |
 *        |                                     |
 *        +------------------+------------------+
 *                           |
 *        +------------------+------------------+
 *        |                                     |
 *    interface                                 |
 *        |                                     |
 *       AAA                                    |
 *        |                                     |
 *        +------------------+------------------+
 *                           |
 *                         vlan
 *                           |
 *                          BBB
 */

int cmd_sh_mac_addr_t(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_mac_age(struct cli_context *, int, char **, struct menu_node **);
extern struct menu_node output_modifiers;

static struct menu_node vlan = {
	.name			= "vlan",
	.help			= "VLAN keyword",
	.mask			= PRIV(1),
	.tokenize	= NULL,
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		& (struct menu_node) {
			.name			= "<1-1094>",
			.help			= "Vlan interface number",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_sh_mac_addr_t,
			.subtree	= (struct menu_node *[]) {
				&output_modifiers,
				NULL
			}
		},

		NULL
	} /*}}}*/
};

static struct menu_node interface = {
	.name			= "interface",
	.help			= "interface keyword",
	.mask			= PRIV(1),
	.tokenize	= NULL, /* tok_interface, */
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		NULL // FIXME
	} /*}}}*/
};

static struct menu_node address = {
	.name			= "address",
	.help			= "address keyword",
	.mask			= PRIV(1),
	.tokenize	= NULL,
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		& (struct menu_node) {
			.name			= "H.H.H",
			.help			= "48 bit mac address",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_sh_mac_addr_t,
			.subtree	= (struct menu_node *[]) {
				&interface,
				&vlan,
				NULL
			}
		},

		NULL
	} /*}}}*/
};

struct menu_node *show_mac_addr_t[] = {
	&address,

	/* #show mac address-table aging-time */
	& (struct menu_node) {
		.name			= "aging-time",
		.help			= "aging-time keyword",
		.mask			= PRIV(1),
		.tokenize	= NULL,
		.run			= cmd_sh_mac_age,
		.subtree	= (struct menu_node *[]) {
			&output_modifiers,
			NULL
		}
	},

	/* #show mac address-table dynamic */
	& (struct menu_node) { 
		.name			= "dynamic",
		.help			= "dynamic entry type",
		.mask			= PRIV(1),
		.tokenize	= NULL,
		.run			= cmd_sh_mac_addr_t,
		.subtree	= (struct menu_node *[]) {
			&address,
			&interface,
			&vlan,
			&output_modifiers,
			NULL
		}
	},

	&interface,

	/* #show mac address-table static */
	& (struct menu_node) { 
		.name			= "static",
		.help			= "static entry type",
		.mask			= PRIV(1),
		.tokenize	= NULL,
		.run			= cmd_sh_mac_addr_t,
		.subtree	= (struct menu_node *[]) {
			&address,
			&interface,
			&vlan,
			&output_modifiers,
			NULL
		}
	},

	&vlan,

	&output_modifiers,

	NULL
};

/**
 * Don't export __show_mac_addr_t directly, because it's a pointer array
 * and its storage size is larger than pointer; outside it's seen as
 * double pointer and we'd get a warning at link time
 */

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/

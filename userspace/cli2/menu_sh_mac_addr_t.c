#include "cli.h"
#include "swcli_common.h"
#include "menu_interface.h"

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

static struct menu_node *vlan_subtree[] = {
	&output_modifiers,
	NULL
};
static struct menu_node vlan =
VLAN_MENU_NODE(vlan_subtree, cmd_sh_mac_addr_t, NULL);

static struct menu_node *if_nexttree[] = {
	&vlan,
	&output_modifiers,
	NULL
};

static struct menu_node *if_subtree[] = {
	IF_ETHER(if_nexttree, cmd_sh_mac_addr_t, NULL),
	IF_NETDEV(if_nexttree, cmd_sh_mac_addr_t, NULL),
	NULL
};

static struct menu_node interface = IF_MENU_NODE(if_subtree, "interface keyword");

static struct menu_node *address_subtree[] = {
	&interface,
	&vlan,
	&output_modifiers,
	NULL
};
static struct menu_node address =
IF_ADDRESS_MENU_NODE(address_subtree, cmd_sh_mac_addr_t, NULL);

struct menu_node *show_mac_addr_t[] = {
	&address,

	/* #show mac address-table aging-time */
	& (struct menu_node) {
		.name			= "aging-time",
		.help			= "aging-time keyword",
		.mask			= CLI_MASK(PRIV(1)),
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
		.mask			= CLI_MASK(PRIV(1)),
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
		.mask			= CLI_MASK(PRIV(1)),
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

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/

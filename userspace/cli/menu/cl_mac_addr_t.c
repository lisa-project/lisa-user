#include "swcli.h"
#include "main.h"

/*                         clear
 *                           |
 *        +------------------+------------------+
 *        |                                     |
 *       mac                                    |
 *        |                             mac-address-table
 *  address-table                               |
 *        |                                     |
 *        +------------------+------------------+
 *                           |
 *                        dynamic
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

static struct menu_node vlan =
VLAN_MENU_NODE(NULL, cmd_cl_mac_addr_t, NULL);

static struct menu_node *if_nexttree[] = {
	&vlan,
	NULL
};

static struct menu_node *if_subtree[] = {
	IF_ETHER(if_nexttree, cmd_cl_mac_addr_t, NULL),
	IF_NETDEV(if_nexttree, cmd_cl_mac_addr_t, NULL),
	NULL
};

static struct menu_node interface = IF_MENU_NODE(if_subtree, "interface keyword");

static struct menu_node *address_subtree[] = {
	&interface,
	&vlan,
	NULL
};
static struct menu_node address =
IF_ADDRESS_MENU_NODE(address_subtree, cmd_cl_mac_addr_t, NULL);

struct menu_node *clear_mac_addr_t[] = {
	/* #clear mac address-table dynamic */
	& (struct menu_node) { 
		.name			= "dynamic",
		.help			= "dynamic entry type",
		.mask			= CLI_MASK(PRIV(1)),
		.tokenize	= NULL,
		.run			= cmd_cl_mac_addr_t,
		.subtree	= (struct menu_node *[]) {
			&address,
			&interface,
			&vlan,
			NULL
		}
	},

	NULL
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/

#ifndef _MENU_INTERFACE_H
#define _MENU_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

#include "cli.h"
#include "if_generic.h"

#define IF_MENU_NODE(__subtree, __help, __priv...) {\
	.name		= "interface",\
	.help		= __help,\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= if_tok_if,\
	.run		= NULL,\
	.subtree	= __subtree\
}

#define IF_ETHER_MENU_NODE(__subtree, __run, __tokenize, __priv...) {\
	.name		= "Ethernet",\
	.help		= "Ethernet IEEE 802.3",\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= swcli_tokenize_number,\
	.run		= NULL,\
	.priv		= (void *)1,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name   	= "<0-24>",\
			.help   	= "Ethernet interface number",\
			.mask   	= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
			.tokenize	= __tokenize,\
			.run    	= __run,\
			.priv		= (int []) {VALID_LIMITS, 0, 24},\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}
#define IF_ETHER(...) & (struct menu_node) IF_ETHER_MENU_NODE(__VA_ARGS__)

#define IF_NETDEV_MENU_NODE(__subtree, __run, __tokenize, __priv...) {\
	.name		= "netdev",\
	.help		= "Linux NetDev",\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= swcli_tokenize_word,\
	.run		= NULL,\
	.priv		= (void *)0,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name   	= "WORD",\
			.help   	= "NetDev name",\
			.mask   	= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
			.tokenize	= __tokenize,\
			.run    	= __run,\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}
#define IF_NETDEV(...) & (struct menu_node) IF_NETDEV_MENU_NODE(__VA_ARGS__)

#define VLAN_MENU_NODE(__subtree, __run, __tokenize, __priv...) {\
	.name		= "vlan",\
	.help		= "VLAN keyword",\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= swcli_tokenize_number,\
	.run		= NULL,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name		= "<1-4094>",\
			.help		= "Vlan number",\
			.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
			.tokenize	= __tokenize,\
			.run		= __run,\
			.priv		= (int []) {VALID_LIMITS, 1, 4094},\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}

#define IF_VLAN_MENU_NODE(__subtree, __run, __tokenize, __priv...) {\
	.name		= "vlan",\
	.help		= "LMS Vlans",\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= swcli_tokenize_number,\
	.run		= NULL,\
	.priv		= (void *)1,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name		= "<1-4094>",\
			.help		= "Vlan interface number",\
			.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
			.tokenize	= __tokenize,\
			.run		= __run,\
			.priv		= (int []) {VALID_LIMITS, 1, 4094},\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}
#define IF_VLAN(...) & (struct menu_node) IF_VLAN_MENU_NODE(__VA_ARGS__)

#define IF_ADDRESS_MENU_NODE(__subtree, __run, __tokenize, __priv...) {\
	.name		= "address",\
	.help		= "address keyword",\
	.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
	.tokenize	= swcli_tokenize_mac,\
	.run		= NULL,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name		= "H.H.H",\
			.help		= "48 bit mac address",\
			.mask		= CLI_MASK(VA_PRIV(NIL, ##__priv, 1)),\
			.tokenize	= __tokenize,\
			.run		= __run,\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}
#define IF_ADDRESS(...) & (struct menu_node) IF_ADDRESS_MENU_NODE(__VA_ARGS__)

/* all known interface types */
enum {
	IF_T_ERROR,
	IF_T_ETHERNET,
	IF_T_VLAN,
	IF_T_NETDEV
};

/* interface type filters; must not collide with PRIV mask filter */
#define IFF_SWITCHED	((uint32_t)1 << PRIV_SHIFT)
#define IFF_ROUTED		((uint32_t)2 << PRIV_SHIFT)
#define IFF_VIF			((uint32_t)4 << PRIV_SHIFT)

/* Interface type/identifier tokenizer. It calls the standard cli
 * tokenizer, but adds logic to make input like "eth1" be treated as
 * "Ethernet 1" rather than a partial match of the "Ethernet" node.
 */
int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

/* Parse cli args denoting an interface "extended" name in Cisco format:
 * interface type (Ethernet, vlan etc) followed by interface identifier
 * (number, card/number etc).
 *
 * The real linux netdevice name is returned in name, which must point
 * to a buffer at least IFNAMSIZ bytes long.
 *
 * If n is not null, it will be filled up with the interface identifier
 * (e.g. vlan number for vlan interfaces). For the special "netdev"
 * interface type, n will be filled with -1.
 *
 * The function returns the interface type, which is one of the constants
 * in the interface type enum above. For an unknown interface type or a
 * netdevice with invalid name (longer than IFNAMSIZ), IF_T_ERROR is
 * returned. To distinguish between the two cases, one must look at n,
 * which is 0 for unknown interface type and -1 for invalid netdev name.
 */
int if_parse_args(char **argv, struct menu_node **nodev, char *name, int *n);

#define __if_args_to_ifindex(__ctx, __argv, __nodev, __sock_fd, __index, __type, __name, ...) do {\
	int n;\
	char __if_tmp_name[IFNAMSIZ];\
	char *name = (__name) ? (__name) : &__if_tmp_name[0];\
\
	__type = if_parse_args(__argv, __nodev, name, &n);\
\
	if (__type == IF_T_ERROR) {\
		if (n == -1)\
			EX_STATUS_REASON(__ctx, "invalid interface name");\
		else \
			(__ctx)->ex_status.reason = NULL;\
		SW_SOCK_CLOSE(__ctx, __sock_fd);\
		return CLI_EX_REJECTED;\
	}\
\
	if (!(__index = if_get_index(name, __sock_fd))) {\
		EX_STATUS_REASON(__ctx, "interface %s does not exist", name);\
		SW_SOCK_CLOSE(__ctx, __sock_fd);\
		return CLI_EX_REJECTED;\
	}\
} while (0)

#define if_args_to_ifindex(__ctx, __argv, __nodev, __sock_fd, __index, __type, __name...) \
	__if_args_to_ifindex(__ctx, __argv, __nodev, __sock_fd, __index, __type, ##__name, NULL)

#define if_get_type(__ctx, __sock_fd, __index, __name, __type, __vlan) do {\
	if (sw_ops->if_get_type(sw_ops, __index, &(__type), &(__vlan))) {\
		EX_STATUS_REASON_IOCTL(__ctx, errno);\
		SW_SOCK_CLOSE(__ctx, __sock_fd);\
		return CLI_EX_REJECTED;\
	}\
	if (__type == IF_TYPE_NONE) {\
		EX_STATUS_REASON(__ctx, "interface %s not in switch", __name);\
		SW_SOCK_CLOSE(__ctx, __sock_fd);\
		return CLI_EX_REJECTED;\
	}\
} while(0)
#endif

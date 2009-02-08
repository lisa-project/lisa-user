#ifndef _MENU_INTERFACE_H
#define _MENU_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

#include "cli.h"

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
			.name		= "<1-1094>",\
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
			.name		= "<1-1094>",\
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

int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

static inline int if_name_generic(char *name, const char *type, const char *num) {
	int status;
	int n = atoi(num);

	/* convert forth and back to int to avoid 0-left-padded numbers */
	status = snprintf(name, IFNAMSIZ, "%s%d", type, n);

	/* use assert() since number range is validated through cli */
	assert(status < IFNAMSIZ);

	return n;
}
#define if_name_ethernet(name, num) if_name_generic(name, "eth", num)
#define if_name_vlan(name, num) if_name_generic(name, "vlan", num)

int if_parse_generic(const char *name, const char *type);
#define if_parse_vlan(name) if_parse_generic(name, "vlan");


int if_parse_args(char **argv, struct menu_node **nodev, char *name, int *n);

int if_get_index(const char *name, int sock_fd);

#endif

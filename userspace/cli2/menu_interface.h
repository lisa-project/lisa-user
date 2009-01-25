#ifndef _MENU_INTERFACE_H
#define _MENU_INTERFACE_H

#define IF_MENU_NODE(__subtree, __priv...) {\
	.name		= "interface",\
	.help		= "interface keyword",\
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
	.tokenize	= NULL,\
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

/* interface type filters; must not collide with PRIV mask filter */
#define IFF_SWITCHED	((uint32_t)1 << PRIV_SHIFT)
#define IFF_ROUTED		((uint32_t)2 << PRIV_SHIFT)
#define IFF_VIF			((uint32_t)4 << PRIV_SHIFT)

int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

int parse_vlan(char *buf);
#endif

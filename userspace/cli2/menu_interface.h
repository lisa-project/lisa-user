#ifndef _MENU_INTERFACE_H
#define _MENU_INTERFACE_H

#define IF_PRIV(NIL, priv, ...) PRIV(priv)

#define IF_MENU_NODE(__subtree, __priv...) {\
	.name		= "interface",\
	.help		= "interface keyword",\
	.mask		= IF_PRIV(NIL, ##__priv, 1),\
	.tokenize	= if_tok_if,\
	.run		= NULL,\
	.subtree	= __subtree\
}

#define IF_ETHER(__subtree, __run, __tokenize, __priv...) & (struct menu_node) {\
	.name		= "Ethernet",\
	.help		= "Ethernet IEEE 802.3",\
	.mask		= IF_PRIV(NIL, ##__priv, 1),\
	.tokenize	= swcli_tokenize_number,\
	.run		= NULL,\
	.priv		= (void *)1,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name   	= "<0-24>",\
			.help   	= "Ethernet interface number",\
			.mask   	= IF_PRIV(NIL, ##__priv, 1),\
			.tokenize	= __tokenize,\
			.run    	= __run,\
			.priv		= (int []) {VALID_LIMITS, 0, 24},\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}

#define IF_NETDEV(__subtree, __run, __tokenize, __priv...) & (struct menu_node) {\
	.name		= "netdev",\
	.help		= "Linux NetDev",\
	.mask		= IF_PRIV(NIL, ##__priv, 1),\
	.tokenize	= swcli_tokenize_word,\
	.run		= NULL,\
	.priv		= (void *)0,\
	.subtree	= (struct menu_node *[]) {\
		& (struct menu_node) {\
			.name   	= "WORD",\
			.help   	= "NetDev name",\
			.mask   	= IF_PRIV(NIL, ##__priv, 1),\
			.tokenize	= __tokenize,\
			.run    	= __run,\
			.subtree	= __subtree\
		},\
		\
		NULL\
	}\
}

int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

#endif

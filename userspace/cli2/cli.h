#ifndef _CLI_H
#define _CLI_H

#include <stdio.h>

struct menu_node;

#define TOKENIZE_MAX_MATCHES 128

#ifndef whitespace
#define whitespace(c) ((c) == ' ' || (c) == '\t')
#endif

/* Tokenizer output */
struct tokenize_out {
	/* Offset of the current token in the input buffer */
	int offset;
	/* Length of the current token */
	int len;
	/* Array of matching menu nodes */
	struct menu_node *matches[TOKENIZE_MAX_MATCHES+1];
};

/* Command tree menu node */
struct menu_node {
	/* Complete name of the menu node */
	const char *name;
	/* Help message */
	const char *help;
	/* Minimum privilege level to access the node */
	int priv;
	/* Custom tokenize function for the node */
	int (*tokenize)(const char *buf, struct menu_node *tree, struct tokenize_out *out);
	/* Command handler for runnable nodes */
	int (*run)(FILE *out, int argc, char **tokv, struct menu_node **nodev);
	/* Points to the sub menu of the node */
	struct menu_node *subtree;
};

#define NULL_MENU_NODE {\
	.name		= NULL,\
	.help		= NULL,\
	.priv		= 0,\
	.tokenize	= NULL,\
	.run		= NULL,\
	.subtree	= NULL\
}

int cli_next_token(const char *buf, struct tokenize_out *out);
int cli_tokenize(const char *buf, struct menu_node *tree, struct tokenize_out *out);

#endif

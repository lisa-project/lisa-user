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

struct cli_context {
	int filter;
	struct menu_node *root;
};

/* Command tree menu node */
struct menu_node {
	/* Complete name of the menu node */
	const char *name;

	/* Help message */
	const char *help;

	/* Bitwise mask for filtering */
	int mask;

	/* Custom tokenize function for the node */
	int (*tokenize)(struct cli_context *ctx, const char *buf, struct menu_node *tree, struct tokenize_out *out);

	/* Command handler for runnable nodes; ctx is a passed-back pointer,
	 * initially sent as argument to cli_exec(); argc is the number of
	 * tokens in the command; tokv is the array of tokens (exactly as
	 * they appear in the input command); nodev is the array of matching
	 * menu nodes, starting from root.
	 */
	int (*run)(struct cli_context *ctx, int argc, char **tokv, struct menu_node **nodev);

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
int cli_tokenize(struct cli_context *ctx, const char *buf, struct menu_node *tree, struct tokenize_out *out);
int cli_exec(struct cli_context *ctx, char *cmd);

#endif

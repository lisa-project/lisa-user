#define _GNU_SOURCE
#include <string.h>

#include "cli.h"

int cli_next_token(const char *buf, struct tokenize_out *out)
{
	const char *delim = " \t";

	out->len = 0;

	/* buf contains only whitespace */
	out->offset = strspn(buf, delim);
	if (out->offset == strlen(buf)) {
		out->matches[0] = NULL;
		return -1;
	}

	/* skip whitespace */
	buf += out->offset;

	/* look for next whitespace */
	out->len = strcspn(buf, delim);

	return 0;
}

int cli_tokenize(const char *buf, struct menu_node *tree, struct tokenize_out *out)
{
	char *token, c;
	int i, j;

	/* get next token */
	if (cli_next_token(buf, out))
		return 0;

	token = strdupa(buf) + out->offset;
	c = token[out->len];
	token[out->len] = '\0';

	/* lookup token in tree */
	for (i=0, j=0; tree[i].name && j < TOKENIZE_MAX_MATCHES; i++) {
		/* register matches */
		if (!(strncmp(token, tree[i].name, out->len)))
			out->matches[j++] = &tree[i];
	}
	out->matches[j] = NULL;

	/* Bad state when we have a whitespace after the token
	 * and multiple matches. */
	return (whitespace(c) && (j > 1));
}

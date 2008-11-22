#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
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

int cli_tokenize(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out)
{
	char *token, c;
	int i, j;

	/* always reset ok_len to 0 */
	out->ok_len = 0;
	out->exact_match = NULL;

	/* get next token */
	if (cli_next_token(buf, out))
		return 0;

	c = buf[out->offset + out->len];

	/* allow "matching" against an empty subtree; this eases detection of
	 * junk after a token that matches the deepest subtree node */
	if (tree == NULL) {
		out->matches[0] = NULL;
		return whitespace(c);
	}

	token = strdupa(buf + out->offset);
	token[out->len] = '\0';

	/* lookup token in tree */
	for (i=0, j=0; tree[i] && j < TOKENIZE_MAX_MATCHES; i++) {
		/* apply filter */
		if ((tree[i]->mask & ctx->filter) != tree[i]->mask)
			continue;

		if (strncmp(token, tree[i]->name, out->len))
			continue;

		/* register match */
		out->matches[j++] = tree[i];

		/* check for exact match */
		if (out->len == strlen(tree[i]->name))
			out->exact_match = tree[i];
	}
	out->matches[j] = NULL;

	/* If no matches found, determine ok_len. At first iteration, j is used
	 * as match count (no iteration if we have any matches); at subsequent
	 * iterations, it is used as sentinel to break when ok_len is determined */
	for (out->ok_len = out->len; out->ok_len && !j; ) {
		token[--(out->ok_len)] = '\0';
		if (!out->ok_len)
			break;
		for (i = 0; tree[i]; i++) {
			if (strncmp(token, tree[i]->name, out->ok_len))
				continue;
			j = 1;
			break;
		}
	}

	/* Bad state when we have a whitespace after the token
	 * and multiple matches. */
	return whitespace(c);
}

int cli_exec(struct cli_context *ctx, char *buf)
{
	struct tokenize_out out;
	int size, trailing_whitespace;
	struct menu_node *menu = ctx->root;
	char *cmd = buf;
	int argc = 0;
	char *tokv[MAX_MENU_DEPTH];
	struct menu_node *nodev[MAX_MENU_DEPTH];

	for (;;) {
		assert(argc < MAX_MENU_DEPTH);

		trailing_whitespace = menu->tokenize == NULL ?
			cli_tokenize(ctx, cmd, menu->subtree, &out) :
			menu->tokenize(ctx, cmd, menu->subtree, &out);

		/* A single exact match will have priority over other
		 * partial matches. */
		if (out.exact_match != NULL) {
			out.matches[0] = out.exact_match;
			out.matches[1] = NULL;
		}
		size = MATCHES(&out);

		/* Case A: 2 or more matches */
		if (size > 1)
			return CLI_EX_AMBIGUOUS;

		/* Case B: exactly 1 match */
		if (size == 1) {
			tokv[argc] = strndupa(cmd + out.offset, out.len);
			nodev[argc] = out.matches[0];
			argc++;
			menu = out.matches[0];
			/* continue parsing if we have a whitespace after the token */
			if (trailing_whitespace) {
				cmd += out.offset + out.len;
				continue;
			}
			break;
		}

		/* Case C: no matches, trailing whitespace. try to run the command  */
		if (!out.len)
			break;

		/* Case D: no matches, extra garbage at the end of the line */
		return (((cmd - buf) + out.offset + out.ok_len)  << CLI_EX_STAT_BITS)
			| CLI_EX_INVALID;
	}

	/* If control reaches this point, we must run the command. */
	return menu->run? menu->run(ctx, argc, tokv, nodev) : CLI_EX_INCOMPLETE;
}

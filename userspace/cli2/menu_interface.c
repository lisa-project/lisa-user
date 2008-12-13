#include "cli.h"

int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	int ret = cli_tokenize(ctx, buf, tree, out);

	if (out->partial_match && out->partial_match->priv) {
		out->matches[0] = out->partial_match;
		out->matches[1] = NULL;

		out->partial_match = NULL;
		out->len = out->ok_len;
		ret = 1;
	}

	return ret;
}


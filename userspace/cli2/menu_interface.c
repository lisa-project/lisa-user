#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if.h>

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

int parse_vlan(char *buf) {
	int ret, n;

	if (!sscanf(buf, "vlan%d%n", &ret, &n))
		return -1;

	if (strlen(buf) != n)
		return -1;

	return ret;
}

#include "cli.h"
#include "swcli_common.h"

int addr_tokenize(struct cli_context *cc, const char *buf, struct menu_node *tree, struct tokenize_out *out) {
	int i = 0;

	if (cli_next_token(buf, out))
		return 0;

	if (out->len >= 4) {
		out->matches[0] = &tree[0];
		out->matches[1] = NULL;
		return 0;
	}

	if (!whitespace(buf[out->offset + out->len]))
		out->matches[i++] = &tree[0];

	out->matches[i] = NULL;

	return 1;
}

struct menu_node root[] = {
	{ // path = /
		.name		= "ip",
		.subtree	= (struct menu_node[]) {
			{ // path = /ip
				.name		= "access-group"
			},
			{ // path = /ip
				.name		= "address",
				.subtree	= (struct menu_node[]) {
					{ // path = /ip/addr
						.name		= "ABCD"
					},
					NULL_MENU_NODE
				},
				.tokenize	= addr_tokenize
			},
			NULL_MENU_NODE
		}
	},
	{ // path = /
		.name		= "show",
		.subtree	= (struct menu_node[]) {
			{ // path = /show
				.name		= "version"
			},
			NULL_MENU_NODE
		}
	},
	NULL_MENU_NODE
};

char * test[] = {
	"ip a",
	"ip a ",
	"ip addr 123",
	"ip addr 123 ",
	"ip addr 1234",
	"ip addr 1234 ",
	"ip jjjj",
	"sh ver",
	"sh ver ",
	NULL
};

void test_cmd(struct cli_context *ctx, char *cmd) {
	struct tokenize_out out;
	int status, size, step = 0;
	struct menu_node *menu = root;
	int (*tokenize)(struct cli_context *ctx, const char *buf, struct menu_node *tree, struct tokenize_out *out) = cli_tokenize;

	printf("Command: '%s'\n", cmd);

	do {
		step++;
		status = tokenize(ctx, cmd, menu, &out);
		if (out.matches[0]) {
			menu = out.matches[0]->subtree;
			tokenize = out.matches[0]->tokenize ? out.matches[0]->tokenize : cli_tokenize;
		}
		for (size = 0; out.matches[size]; size++);
		cmd += out.offset + out.len;
	} while (!status && out.len && size == 1);

	printf("  steps=%d ret=%d offset=%d len=%d size=%d\n\n",
			step, status, out.offset, out.len, size);
}

int main(int argc, char **argv) {
	struct cli_context cc;
	char **cmd;

	/*
	printf("%s\n", root[0].name);
	printf("%s\n", root[0].subtree[0].name);
	printf("%s\n", root[0].subtree[1].name);
	printf("%s\n", root[1].subtree[0].name);
	*/
	cc.node_filter = PRIV_FILTER(15);
	cc.root = &root[0];

	for (cmd = test; *cmd; cmd++)
		test_cmd(&cc, *cmd);

	return 0;
}

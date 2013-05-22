#include "swcli.h"

int swcli_dump_args(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) {
	int i;

	for (i = 0; i < argc; i++)
		printf("%2d: %-15s %-15s\n", i, argv[i], nodev[i]->name);
	return 0;
}

char *swcli_prompt(struct rlshell_context *ctx) {
	char hostname[HOST_NAME_MAX + 1];
	size_t buf_size = HOST_NAME_MAX + MENU_NAME_MAX + 3;
	char *buf = malloc(buf_size);
	char prompt = ctx->cc.node_filter & PRIV(2) ? '#' : '>';

	if (buf == NULL)
		return buf;

	gethostname(hostname, HOST_NAME_MAX);
	hostname[HOST_NAME_MAX] = '\0';

	if (ctx->cc.root->name == NULL)
		snprintf(buf, buf_size, "%s%c", hostname, prompt);
	else
		snprintf(buf, buf_size, "%s(%s)%c", hostname,
				ctx->cc.root->name, prompt);

	return buf;
}

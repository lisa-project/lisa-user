#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cli.h"
#include "swcli_common.h"

extern struct menu_node config_main;

int dump_args(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) {
	int i;

	for (i = 0; i < argc; i++)
		printf("%2d: %-15s %-15s\n", i, argv[i], nodev[i]->name);
	return 0;
}

int swcli_tokenize_line(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	struct rlshell_context *rlctx = (void *)ctx;

	if (cli_next_token(buf, out))
		return 0;

	out->matches[0] = tree[0];
	out->matches[1] = NULL;
	rlctx->suppress_completion = 1;

	return 1;
}

static inline int swcli_valid_number(int *valid, int val)
{
	int i;

	if (!valid)
		return 1;

	switch (valid[0]) {
	case VALID_LIMITS:
		return val >= valid[1] && val <= valid[2];
	case VALID_LIST:
		for (i = 2; i < valid[1] + 2; i++)
			if (val == valid[i])
				return 1;
		return 0;
	}

	return 1;
}

int swcli_tokenize_number(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	char c, *token;
	const char *ok = "0123456789";
	int *valid = tree[0]->priv;
	struct rlshell_context *rlctx = (void *)ctx;

	rlctx->suppress_completion = 1;

	if (cli_next_token(buf, out))
		return 0;

	buf += out->offset;

	c = buf[out->len];
	out->matches[0] = NULL;
	out->matches[1] = NULL;

	out->ok_len = strspn(buf, ok);

	if (out->ok_len < out->len) {
		out->partial_match = tree[0];
		return whitespace(c);
	}

	token = strdupa(buf);
	while (out->ok_len) {
		int val;

		token[out->ok_len] = '\0';
		val = atoi(token);
		if (swcli_valid_number(valid, val))
			break;

		out->ok_len--;
	}

	if (out->ok_len < out->len) {
		out->partial_match = tree[0];
		return whitespace(c);
	}

	out->matches[0] = tree[0];
	return whitespace(c);
}

int swcli_tokenize_word(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	char c;
	struct rlshell_context *rlctx = (void *)ctx;

	rlctx->suppress_completion = 1;

	if (cli_next_token(buf, out))
		return 0;

	c = buf[out->offset + out->len];

	out->matches[0] = tree[0];
	out->matches[1] = NULL;

	return whitespace(c);
}

int swcli_output_modifiers_run(struct cli_context *ctx, int argc, char **argv,
		struct menu_node **nodev)
{
	struct cli_filter_priv priv;
	int i, j, err = 0;

	for (i=argc-1; i>=0; i--) {
		if (!nodev[i]->run)
			continue;
		if (nodev[i]->run != swcli_output_modifiers_run)
			break;
	}

	/* check that a valid node was found */
	if (i < 0 || i >= argc-3 || strcmp(argv[i+1], "|")) {
		err = -EINVAL;
		goto out_return;
	}

	/* make i the index of the next node after '|' */
	i+=2;

	memset(&priv, 0, sizeof(priv));

	priv.argv = (const char **)calloc(argc - i + 2, sizeof(char *));
	if (!priv.argv) {
		err = -ENOMEM;
		goto out_return;
	}

	/* copy the necessary argv elements: filter type and verbatim filter
	 * arguments.
	 * argv[0] is intentionally left NULL. It will be filled in later by
	 * the cli.
	 */
	priv.argv[1] = nodev[i]->name;
	for (j = i+1; j<argc; j++)
		priv.argv[j - i + 1] = argv[j];

	/* set context information to be available for the output handler */
	ctx->filter.priv = &priv;
	ctx->filter.open = cli_filter_open;
	ctx->filter.close = cli_filter_close;

	/* the real handler knows only about arguments before the '|' */
	nodev[i-2]->run(ctx, i-1, argv, nodev);

	if (priv.argv)
		free(priv.argv);

out_return:
	return err;
}

int cmd_clr_mac(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_clr_mac_adr(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_clr_mac_eth(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_clr_mac_vl(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_conf_t(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct rlshell_context *ctx = (void *)__ctx;

	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	ctx->cc.root = &config_main;
	ctx->enable_ctrl_z = 1;
	return 0;
}

int cmd_disable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_enable(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_quit(struct cli_context *__ctx, int argc, char **argv, struct menu_node **nodev) {
	struct rlshell_context *ctx = (void *)__ctx;

	ctx->exit = 1;
	dump_args(__ctx, argc, argv, nodev);
	return 0;
}

int cmd_help(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_ping(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_reload(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_entry(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_ne(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){
	dump_args(ctx, argc, argv, nodev);
	return 0;
}
int cmd_sh_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_traffic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_history(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	FILE *out;
	int i;

	out = ctx->out_open(ctx, 1);
	for (i=0; i<1000; i++)
		fprintf(out, "%d\n", i);
	fflush(out);
	fclose(out);
	return 0;
}

int cmd_sh_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_ip(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_addr(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_age(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_eth(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_mac_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_priv(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_run_if(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_start(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_ver(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_trace(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_wrme(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

int cmd_sh_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){
	printf("%s: called\n", __func__);
	dump_args(ctx, argc, argv, nodev);
	return 0;
}

int cmd_cl_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){
	printf("%s: called\n", __func__);
	dump_args(ctx, argc, argv, nodev);
	return 0;
}

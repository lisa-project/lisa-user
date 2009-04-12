#include "swcli.h"

/*
 * FIXME: all functions that need not be visible from outside must be
 * declared static. Must decide what's the public API exposed by this lib.
 */
static int swcli_valid_number(const char *token, int part, void *priv)
{
	int i, val;
	int *valid = priv;

	if (!valid)
		return 1;

	assert(token);

	for (i = 0; token[i] != '\0'; i++)
		if (token[i] < '0' || token[i] > '9')
			return 0;

	val = atoi(token);

	// FIXME implement tests for part == 1
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

#define IS_HEX_DIGIT(c) \
	(((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

static int valid_mac(const char *token, int part, void *priv)
{
	int digits = 0, group = 1, i;

	for (i = 0; token[i] != '\0'; i++) {
		if (token[i] == '.') {
			if (!digits)
				return 0;
			digits = 0;
			group ++;
			continue;
		}
		if (!IS_HEX_DIGIT(token[i]))
			return 0;
		if (++digits > 4)
			return 0;
	}

	if (group > 3)
		return 0;

	return part || group == 3;
}

static int valid_ip(const char *token, int part, void *priv)
{
	int x = 0, digits = 0, group = 1, i;

	for (i = 0; token[i] != '\0'; i++) {
		if (token[i] == '.') {
			if (!digits)
				return 0;
			digits = 0;
			x = 0;
			group ++;
			continue;
		}
		if (token[i] < '0' || token[i] > '9')
			return 0;
		++digits;
		if ((x = x * 10 + (token[i] - '0')) > 255)
			return 0;
	}

	if (group > 4)
		return 0;

	return part || group == 4;
}

/* Accept any character sequence, including whitespace */
int swcli_tokenize_line(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	struct rlshell_context *rlctx = (void *)ctx;

	if (cli_next_token(buf, out))
		return 0;
	out->len = strlen(buf) - out->offset;

	out->matches[0] = rlctx->state == RLSHELL_COMPLETION ? NULL : tree[0];
	out->matches[1] = NULL;

	/* This is always the last token (it CAN contain whitespace) */
	return 0;
}

/* Accept any single WORD (no whitespace) and suppress completion */
int swcli_tokenize_word(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	int ws;
	struct rlshell_context *rlctx = (void *)ctx;

	if (cli_next_token(buf, out))
		return 0;

	ws = whitespace(buf[out->offset + out->len]);

	/* To prevent completion, return no matches if we are at the
	 * token (no trailing whitespace) */
	out->matches[0] = rlctx->state == RLSHELL_COMPLETION && !ws ? NULL : tree[0];
	out->matches[1] = NULL;

	return ws;
}

/* Accept any single WORD (no whitespace) OR any subnode other than
 * WORD. Subnode names have priority over WORD. WORD is selected as
 * match only if no other subnode matches. In this case, suppress
 * completion. */
int swcli_tokenize_mixed(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out,
		const char *word, int enable_ws)
{
	char *token;
	int i, j, ws;
	struct rlshell_context *rlctx = (void *)ctx;

	/* get next token */
	if (cli_next_token(buf, out))
		return 0;

	ws = whitespace(buf[out->offset + out->len]);

	token = strdupa(buf + out->offset);
	token[out->len] = '\0';

	/* lookup token in tree */
	for (i=0, j=0; tree[i] && j < TOKENIZE_MAX_MATCHES; i++) {
		/* apply filter */
		if (!cli_mask_apply(tree[i]->mask, ctx->node_filter))
			continue;

		if (!strcmp(tree[i]->name, word)) {
			if (rlctx->state != RLSHELL_COMPLETION || (ws && enable_ws))
				out->matches[j++] = tree[i];
			continue;
		}

		if (strncasecmp(token, tree[i]->name, out->len))
			continue;

		/* register match */
		out->matches[j++] = tree[i];

		/* check for exact match */
		if (out->len == strlen(tree[i]->name))
			out->exact_match = tree[i];
	}
	out->matches[j] = NULL;

	/* ok_len is always equal to token length, since WORD can have an
	 * arbitrary number of characters in length */
	out->ok_len = out->len;

	return enable_ws ? ws : out->exact_match != NULL && ws;
}

int swcli_tokenize_word_mixed(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	return swcli_tokenize_mixed(ctx, buf, tree, out, "WORD", 1);
}

int swcli_tokenize_line_mixed(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	int ws = swcli_tokenize_mixed(ctx, buf, tree, out, "LINE", 0);

	if (MATCHES(out) == 1 && !strcmp(out->matches[0]->name, "LINE"))
		out->len = strlen(buf) - out->offset;

	/* FIXME to ressemble Cisco exactly, this is the point where we
	 * would suppress an exact match (reset out->exact_match to NULL)
	 * if and only if we are invoked by command execution AND there
	 * is no next token. Example: "enable secret 0 " is ambiguous
	 * only for command execution; otherwise (inline help or
	 * completion) node "0" is an exact match and calling function
	 * advances to subnode. */

	return ws;
}

/**
 * Generic tokenizer based on string validator callback.
 */
int swcli_tokenize_validator(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out, int (*valid)(const char *, int, void *), void *valid_priv)
{
	char *token;
	struct rlshell_context *rlctx = (void *)ctx;
	int ws, part;

	if (cli_next_token(buf, out))
		return 0;

	buf += out->offset;

	ws = whitespace(buf[out->len]);
	out->matches[0] = NULL;
	out->matches[1] = NULL;

	if (rlctx->state == RLSHELL_COMPLETION && !ws)
		return ws;

	out->ok_len = out->len;
	token = strdupa(buf);
	part = !ws && rlctx->state != RLSHELL_EXEC;

	while (out->ok_len) {
		token[out->ok_len] = '\0';

		if (valid(token, part, valid_priv))
			break;

		out->ok_len--;
		part = 1;
	}

	if (out->ok_len < out->len) {
		out->partial_match = tree[0];
		return ws;
	}

	out->matches[0] = tree[0];
	return ws;
}

int swcli_tokenize_number(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	return swcli_tokenize_validator(ctx, buf, tree, out, swcli_valid_number, tree[0]->priv);
}

int swcli_tokenize_mac(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	return swcli_tokenize_validator(ctx, buf, tree, out, valid_mac, NULL);
}

int swcli_tokenize_ip(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	return swcli_tokenize_validator(ctx, buf, tree, out, valid_ip, NULL);
}

#include "swcli.h"
#include "main.h"

extern struct menu_node config_main;

int dump_args(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev) {
	int i;

	for (i = 0; i < argc; i++)
		printf("%2d: %-15s %-15s\n", i, argv[i], nodev[i]->name);
	return 0;
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

static int parse_mac(const char *str, unsigned char *mac)
{
	int a, b, c, n;

	if (sscanf(str, "%x.%x.%x%n", &a, &b, &c, &n) != 3)
		return EINVAL;
	if (strlen(str) != n)
		return EINVAL;

	mac[0] = (a & 0xff00) >> 8;
	mac[1] = (a & 0x00ff) >> 0;
	mac[2] = (b & 0xff00) >> 8;
	mac[3] = (b & 0x00ff) >> 0;
	mac[4] = (c & 0xff00) >> 8;
	mac[5] = (c & 0x00ff) >> 0;

	return 0;
}

#define IS_HEX_DIGIT(c) \
	(((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

int valid_mac(const char *token, int part, void *priv)
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

int valid_ip(const char *token, int part, void *priv)
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

// FIXME move to appropriate place
int swcli_tokenize_ip(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	return swcli_tokenize_validator(ctx, buf, tree, out, valid_ip, NULL);
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

static __inline__ void init_mac_filter(struct swcfgreq *swcfgr) {
	swcfgr->ifindex = 0;
	memset(&swcfgr->ext.mac.addr, 0, ETH_ALEN);
	swcfgr->ext.mac.type = SW_FDB_ANY;
	swcfgr->vlan = 0;
}

static int parse_mac_filter(struct swcfgreq *swcfgr, struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev, int sock_fd, char *ifname)
{
	int status;

	init_mac_filter(swcfgr);

	if (!argc)
		return 0;

	do {
		if (!strcmp(nodev[0]->name, "static")) {
			swcfgr->ext.mac.type = SW_FDB_STATIC;
			SHIFT_ARG(argc, argv, nodev);
			break;
		}

		if (!strcmp(nodev[0]->name, "dynamic")) {
			swcfgr->ext.mac.type = SW_FDB_DYN;
			SHIFT_ARG(argc, argv, nodev);
			break;
		}
	} while (0);

	if (!argc)
		return 0;

	if (!strcmp(nodev[0]->name, "address")) {
		assert(argc >= 2);
		status = parse_mac(argv[1], swcfgr->ext.mac.addr);
		assert(!status);
		SHIFT_ARG(argc, argv, nodev, 2);
	}

	if (!argc)
		return 0;
	
	if (!strcmp(nodev[0]->name, "interface")) {
		char __name[IFNAMSIZ];
		int n;
		char *name = ifname ? ifname : &__name[0];

		assert(argc >= 3);
		SHIFT_ARG(argc, argv, nodev);

		status = if_parse_args(argv, nodev, name, &n);

		if (status == IF_T_ERROR) {
			if (n == -1)
				EX_STATUS_REASON(ctx, "invalid interface name");
			else 
				ctx->ex_status.reason = NULL;
			return CLI_EX_REJECTED;
		}
		
		if (!(swcfgr->ifindex = if_get_index(name, sock_fd))) {
			EX_STATUS_REASON(ctx, "interface %s does not exist", name);
			return CLI_EX_REJECTED;
		}

		SHIFT_ARG(argc, argv, nodev, 2);
	}

	if (!argc)
		return 0;

	if (!strcmp(nodev[0]->name, "vlan")) {
		assert(argc >= 2);
		swcfgr->vlan = atoi(argv[1]);
	}

	return 0;
}

int cmd_sh_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int ret = CLI_EX_OK;
	int status, sock_fd;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_GETMAC
	};
	char ifname[IFNAMSIZ];

	SW_SOCK_OPEN(ctx, sock_fd);

	assert(argc >= 2);
	SHIFT_ARG(argc, argv, nodev, strcmp(nodev[1]->name, "mac") ? 2 : 3);

	if ((status = parse_mac_filter(&swcfgr, ctx, argc, argv, nodev, sock_fd, ifname))) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return status;
	}

	if ((status = buf_alloc_swcfgr(&swcfgr, sock_fd)) < 0)
		switch (-status) {
		case EINVAL:
			EX_STATUS_REASON(ctx, "interface %s not in switch", ifname);
			ret = CLI_EX_REJECTED;
			break;
		default:
			EX_STATUS_REASON_IOCTL(ctx, status);
			ret = CLI_EX_WARNING;
			break;
		}
	else {
		int size = status;
		struct if_map if_map;
		struct if_map_priv priv = {
			.map = &if_map,
			.sock_fd = sock_fd
		};

		if_map_init(&if_map);

		/* if user asked for mac on specific interface, all results will
		 * have the same ifindex and if_get_name fallback is enough;
		 * otherwise fetch interface list from kernel and init hash */
		if (!swcfgr.ifindex) {
			status = if_map_fetch(&if_map, SW_IF_SWITCHED, sock_fd);
			assert(!status);
			status = if_map_init_ifindex_hash(&if_map);
			assert(!status);
		}

		// FIXME open output
		print_mac(stdout, swcfgr.buf.addr, size, if_map_print_mac, &priv);

		if_map_cleanup(&if_map);
	}

	if (swcfgr.buf.addr != NULL)
		free(swcfgr.buf.addr);
	SW_SOCK_CLOSE(ctx, sock_fd);

	return ret;
}

int cmd_cl_mac_addr_t(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	int status, sock_fd;
	struct swcfgreq swcfgr = {
		.cmd = SWCFG_DELMACDYN
	};

	SW_SOCK_OPEN(ctx, sock_fd);

	assert(argc >= 2);
	SHIFT_ARG(argc, argv, nodev, strcmp(nodev[1]->name, "mac") ? 2 : 3);

	if ((status = parse_mac_filter(&swcfgr, ctx, argc, argv, nodev, sock_fd, NULL))) {
		SW_SOCK_CLOSE(ctx, sock_fd);
		return status;
	}

	status = ioctl(sock_fd, SIOCSWCFG, &swcfgr);
	SW_SOCK_CLOSE(ctx, sock_fd); /* this can overwrite ioctl errno */

	if (status == -1) {
		// FIXME output
		fprintf(stdout, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(stdout);
	}

	return 0;
}

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

int cmd_show_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	return CLI_EX_OK;
}

int cmd_sh_run_if(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_start(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_ver(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_show_vlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_trace(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_wrme(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}


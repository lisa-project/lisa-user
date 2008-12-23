#include <assert.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdlib.h>

#include "rlshell.h"
#include "debug.h"

/* Since readline is based on global variables rather than pass-back
 * pointers, we have no way to pass back the context pointer to our
 * readline handlers. Instead, we make a global variable and we
 * initialize it with the current context before calling readline()
 */
static struct rlshell_context *currctx;


/* Error reporting & stuff */
void rlshell_invalid_cmd(void)
{
	printf("%% Unrecognized command\n");
}

void rlshell_ambiguous_cmd(char *cmd)
{
	printf("%% Ambiguous command: \"%s\"\n", cmd);
}

void rlshell_incomplete_cmd(char *cmd)
{
	printf("%% Incomplete command.\n");
}

void rlshell_extra_input(int off)
{
	char fmt[10];
	
	snprintf(fmt, sizeof(fmt), "%%%dc^\n", off);
	printf(fmt, ' ');
	printf("%% Invalid input detected at '^' marker.\n\n");
}

void rlshell_go_ahead(void)
{
	printf("  <cr>\n");
}

/* Attempt to complete the last token in command line buffer
 * and always move the cursor at the end of the buffer. */
int rlshell_completion(int unused, int key)
{
	struct tokenize_out out;
	int size, trailing_whitespace;
	struct menu_node *menu = currctx->cc.root;
	char *cmd = rl_line_buffer;
	int bell = 1;

	rl_point = rl_end;

	for (;;) {
		currctx->suppress_completion = 0;
		trailing_whitespace = menu->tokenize == NULL ?
			cli_tokenize(&currctx->cc, cmd, menu->subtree, &out) :
			menu->tokenize(&currctx->cc, cmd, menu->subtree, &out);

		/* A single exact match will have priority over other
		 * partial matches. */
		if (out.exact_match != NULL && trailing_whitespace) {
			out.matches[0] = out.exact_match;
			out.matches[1] = NULL;
		}
		size = MATCHES(&out);

		/* Case A: more than one match */
		if (size > 1)
			break;

		/* Case B: exactly one match. Continue parsing the input. */
		if (size == 1) {
			/* When there's no trailing whitespace, we are at the last
			 * token and it's ok to complete. */
			if (!currctx->suppress_completion && !trailing_whitespace) {
				rl_insert_text(out.matches[0]->name + out.len);
				rl_insert_text(" ");
				bell = 0;
				break;
			}
			cmd += out.offset + out.len;
			menu = out.matches[0];
			continue;
		}

		/* Case C: no match or whitespace only
		 * Either way, just do nothing. */
		break;
	}

	if (bell)
		putchar('\a');
	putchar('\n');
	rl_forced_update_display();
	return 0;
}

void rlshell_list_matches_brief(struct tokenize_out *out)
{
	int i;

	for (i = 0; out->matches[i] != NULL; i++) {
		if (i)
			fputs("  ", stdout);

		fputs(out->matches[i]->name, stdout);
	}

	fputs("\n", stdout);
}

void rlshell_list_subtree(struct menu_node *node)
{
	int len = 0;
	struct menu_node **sub;
	char fmt[20];

	/* If node->subtree is NULL, then last token matched the deepest
	 * node in the tree, and we only list <cr> as possible completion. */
	if (node->subtree == NULL) {
		rlshell_go_ahead();
		printf("\n");
		return;
	}

	/* Determine maximum length of command name for nice paging */
	for (sub = node->subtree; *sub != NULL; sub++) {
		int mylen = strlen((*sub)->name);
		len = mylen > len ? mylen : len;
	}

	snprintf(fmt, sizeof(fmt), "  %%-%ds  %%s\n", len);

	for (sub = node->subtree; *sub != NULL; sub++)
		printf(fmt, (*sub)->name, (*sub)->help);

	if (node->run != NULL)
		rlshell_go_ahead();

	printf("\n");
}

/* Help function called when the user pressed '?' in the command line. */
int rlshell_help(int unused, int key)
{
	struct tokenize_out out;
	int size, trailing_whitespace;
	struct menu_node *menu = currctx->cc.root;
	char *cmd = rl_line_buffer;

	printf("%c\n", key);

	for (;;) {
		trailing_whitespace = menu->tokenize == NULL ?
			cli_tokenize(&currctx->cc, cmd, menu->subtree, &out) :
			menu->tokenize(&currctx->cc, cmd, menu->subtree, &out);

		/* A single exact match will have priority over other
		 * partial matches. */
		if (out.exact_match != NULL && trailing_whitespace) {
			out.matches[0] = out.exact_match;
			out.matches[1] = NULL;
		}
		size = MATCHES(&out);

		/* Case A: 2 or more matches and trailing whitespace */
		if (size > 1 && trailing_whitespace) {
			rlshell_ambiguous_cmd(rl_line_buffer);
			break;
		}

		/* Case B: 1 or more matches and no trailing whitespace */
		if (size >= 1 && !trailing_whitespace) {
			rlshell_list_matches_brief(&out);
			break;
		}

		/* Case C: exactly one match. Continue parsing the input. */
		if (size == 1) {
			menu = out.matches[0];
			cmd += out.offset + out.len;
			continue;
		}

		/* Case D: no matches */
		if (out.len) {
			rlshell_invalid_cmd();
			break;
		}

		/* If control reaches this point, we have no matches and a token
		 * consisting of whitespace only */
		rlshell_list_subtree(menu);
		break;
	}

	rl_forced_update_display();
	return 0;
}

int rlshell_exec(struct rlshell_context *ctx, char *buf)
{
	int status;

	switch ((status = cli_exec(&ctx->cc, buf))) {
	case CLI_EX_AMBIGUOUS:
		rlshell_ambiguous_cmd(buf);
		return status;
	case CLI_EX_INCOMPLETE:
		rlshell_incomplete_cmd(buf);
		return status;
	case CLI_EX_INVALID:
		rlshell_extra_input(ctx->plen + ctx->cc.ex_status.offset);
		break;
	default:
		break;
	}

	return status;
}

/* Handler that can be installed on a key sequence */
int rlshell_ignore_keyseq(int i, int j)
{
	return 0;
}

/* Executed on SIGINT (clears rl_line_buffer and forces redisplay) */
void rlshell_sigint_handler(int sig)
{
	printf("\n");
	rl_replace_line("", 1);
	rl_forced_update_display();
}

/* Override some readline defaults */
int rlshell_init(void)
{
	dbg("Initializing rlshell\n");

	/* Allow conditional parsing of ~/.inputrc file */
	rl_readline_name = "rlshell";

	/* Setup to ignore SIGINT and SIGTSTP */
	signal(SIGINT, rlshell_sigint_handler);
	signal(SIGTSTP, SIG_IGN);

	/* Tell the completer we want a hook */
	rl_bind_key('?', rlshell_help);
	rl_bind_key('\t', rlshell_completion);
	rl_set_key("\\C-l", rlshell_ignore_keyseq, rl_get_keymap());
	return 0;
}

int rlshell_main(struct rlshell_context *ctx)
{
	char *cmd = NULL;

	rlshell_init();

	while (!ctx->exit) {
		int hist_append = 1;
		char *prompt = NULL;
		
		if (ctx->prompt != NULL)
			prompt = ctx->prompt(ctx);

		if (cmd != NULL)
			free(cmd);

		currctx = ctx;
		ctx->plen = prompt == NULL ? 1 : strlen(prompt);
		cmd = readline(prompt == NULL ? "#" : prompt);
		if (prompt != NULL)
			free(prompt);

		if (cmd == NULL) {
			printf("\n");
			continue;
		}

		if (cmd[0] == '\0')
			continue;

		if (history_length) {
			HIST_ENTRY *he = history_get(history_length);
			hist_append = he != NULL && strcmp(he->line, cmd);
		}

		if (hist_append)
			add_history(cmd);

		rlshell_exec(ctx, cmd);
	}

	return 0;
}

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
void rlshell_invalid_cmd(void) {
	printf("%% Unrecognized command\n");
}

void rlshell_ambiguous_cmd(char *cmd) {
	printf("%% Ambiguous command: \"%s\"\n", cmd);
}

void rlshell_incomplete_cmd(char *cmd) {
	printf("%% Incomplete command.\n");
}

void rlshell_extra_input(int off) {
	char fmt[10];
	
	snprintf(fmt, sizeof(fmt), "%%%dc^\n", off - 1);
	printf(fmt, ' ');
	printf("%% Invalid input detected at '^' marker.\n\n");
}

void rlshell_go_ahead(void) {
	printf("  <cr>\n");
}

/* If we use the default rl_completer_word_break_characters
 then we totally fuck up completion when we have the following
 characters: \'`@$><=;|&{(, getting some weird behavior for completion.
 So, we set blank to be the only word separator */
char *rlshell_completion_word_break(void) {
	rl_completer_word_break_characters = strdup(" ");
	return NULL;
}

/* Override default readline behavior of printing the list of matches
 * on completion (if there is more than one match). We just go to the
 * next line and do a redisplay of the command line.
 */
void rlshell_display_matches(char **matches, int i, int j) {
	printf("\n");
	rl_forced_update_display();
}

/* Generator function for command completion. state lets us
 * know whether to start from scratch; whithout any state
 * (i.e. state == 0), then we start at the top of the list. */
char *rlshell_generator(const char *text, int state) {
	static int list_index, len;
	const char *name;

	dbg("generator: search_set at 0x%p\n", cmpl_state.search_set);
	
	/* FIXME
	if (! cmpl_state.search_set || !strlen(text) || cmpl_state.no_match_token) 
		return ((char *)NULL);

	// on first call do some initialization
	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	// Return the next name which partially matches from the command list
	for (; (name = cmpl_state.search_set[list_index].name); list_index++) {
        if(cmpl_state.search_set[list_index].priv > priv)
            continue;
		if (strncmp(name, text, len) == 0 && 
				(cmpl_state.search_set[list_index].state & CMPL || !cmpl_state.search_set[list_index].valid)) {
			if (cmpl_state.valid && !(cmpl_state.runnable & PTCNT) && !cmpl_state.cmpl)
				continue;
			list_index++;
			return strdup(name);
		}
	}
	
	// if no matches found then return NULL */
	return ((char *)NULL);
}

/* Attempt to complete on the contents of text. start and end
 * bound the region of rl_line_buffer that contains the word to
 * complete. TEXT is the word to complete. We can use the entire
 * contents of rl_line_buffer in case we want to do some simple
 * parsing. Return the array of matches, or NULL if there aren't any */
char **rlshell_completion(const char *text, int start, int end) {
	char **matches = NULL;

	//FIXME parse_command(strdup(rl_line_buffer), change_search_scope);

	rl_attempted_completion_over = 1;
 	rl_completion_display_matches_hook = rlshell_display_matches;
	matches = rl_completion_matches(text, rlshell_generator);

	if (!matches) /* Force redisplay, even when we have no match */
		rlshell_display_matches(matches, start, end);
	return matches;
}

void list_matches_brief(struct tokenize_out *out) {
	int i;

	for (i = 0; out->matches[i] != NULL; i++) {
		if (i)
			fputs("  ", stdout);

		fputs(out->matches[i]->name, stdout);
	}

	fputs("\n", stdout);
}

void list_subtree(struct menu_node *node) {
	int len = 0;
	struct menu_node *sub;
	char fmt[20];

	/* If node->subtree is NULL, then last token matched the deepest
	 * node in the tree, and we only list <cr> as possible completion. */
	if (node->subtree == NULL) {
		rlshell_go_ahead();
		printf("\n");
		return;
	}

	/* Determine maximum length of command name for nice paging */
	for (sub = node->subtree; sub->name != NULL; sub++) {
		int mylen = strlen(sub->name);
		len = mylen > len ? mylen : len;
	}

	snprintf(fmt, sizeof(fmt), "  %%-%ds  %%s\n", len);

	for (sub = node->subtree; sub->name != NULL; sub++)
		printf(fmt, sub->name, sub->help);

	if (node->run != NULL)
		rlshell_go_ahead();

	printf("\n");
}

/* Help function called when the user pressed '?' in the command line. */
int list_current_options(int something, int key) {
	struct tokenize_out out;
	int status, size;
	struct menu_node *menu = currctx->cc.root;
	char *cmd = rl_line_buffer;

	printf("%c\n", key);

	for (;;) {

		status = menu->tokenize == NULL ?
			cli_tokenize(&currctx->cc, cmd, menu->subtree, &out) :
			menu->tokenize(&currctx->cc, cmd, menu->subtree, &out);
		size = MATCHES(&out);

		/* Phase A: 2 or more matches */
		if (size > 1 && status) {
			rlshell_ambiguous_cmd(rl_line_buffer);
			break;
		}

		if (size > 1) {
			list_matches_brief(&out);
			break;
		}

		/* Phase B: exactly 1 match */
		if (size == 1 && !status) {
			list_matches_brief(&out);
			break;
		}

		if (size == 1) {
			menu = out.matches[0];
			cmd += out.offset + out.len;
			continue;
		}

		/* Phase C: no matches */
		if (out.len) {
			rlshell_invalid_cmd();
			break;
		}

		/* If control reaches this point, we have no matches and a token
		 * consisting of whitespace only */

		list_subtree(menu);
		break;
	}

	rl_forced_update_display();
	return 0;
}

/* Hanlder that can be installed on a key sequence */
int rlshell_ignore_keyseq(int i, int j) {
	return 0;
}

/* Executed on SIGINT (clears rl_line_buffer and forces redisplay) */
void cancel_command(int sig) {
	printf("\n");
	rl_replace_line("", 1);
	rl_forced_update_display();
}

/* Override some readline defaults */
int rlshell_init_readline(void) {
	dbg("Init readline\n");

	/* Allow conditional parsing of ~/.inputrc file */
	rl_readline_name = "SwCli";

	/* Setup to ignore SIGINT and SIGTSTP */
	signal(SIGINT, cancel_command);
	signal(SIGTSTP, SIG_IGN);

	/* Tell the completer we want a crack first */
	rl_attempted_completion_function = rlshell_completion;
	/* FIXME: this doesn't work with older readline (readline-4.3-7) 
	   Used it sucessfuly on the latest readline (5.0)
	 */
	//rl_completion_word_break_hook = rlshell_completion_word_break;
	rl_completer_word_break_characters = strdup(" ");
	rl_bind_key('?', list_current_options);
	rl_set_key("\\C-l", rlshell_ignore_keyseq, rl_get_keymap());
	return 0;
}

int rlshell_main(struct rlshell_context *ctx) {
	char *cmd = NULL;

	rlshell_init_readline();

	while (!ctx->exit) {
		int hist_append = 1;
		char *prompt = NULL;
		
		if (ctx->prompt != NULL)
			prompt = ctx->prompt(ctx);

		if (cmd != NULL)
			free(cmd);

		currctx = ctx;
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

		cli_exec(&ctx->cc, cmd);
	}

	return 0;
}

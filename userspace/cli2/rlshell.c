#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

#include "rlshell.h"
#include "debug.h"

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

/* Help function called when the user pressed '?' in the command line. */
int list_current_options(int something, int key) {
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
		char *prompt = ctx->prompt == NULL ? "#" : ctx->prompt(ctx);

		if (cmd != NULL)
			free(cmd);

		cmd = readline(prompt);
		free(prompt);

		if (cmd == NULL)
			continue;

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
}

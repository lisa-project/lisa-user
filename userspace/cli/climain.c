#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "debug.h"
#include "climain.h"
#include "command.h"
#include "shared.h"

sw_command_root_t *cmd_root = &command_root_main;
static sw_command_t *search_set;
char prompt[MAX_HOSTNAME + 32];
static sw_command_handler handler = NULL;
sw_execution_state_t exec_state;

char eth_range[32]; /* FIXME size */
int sock_fd;

/* Current privilege level */
int priv = 1;

/* If we use the default rl_completer_word_break_characters
 then we totally fuck up completion when we have the following
 characters: \'`@$><=;|&{(, getting some weird behavior for completion.
 So, we set blank to be the only word separator */
char *swcli_completion_word_break() {
	rl_completer_word_break_characters = strdup(" ");
	return NULL;
}

/* Error reporting & stuff */
void swcli_invalid_cmd() {
	printf("%% Unrecognized command\n");
}

void swcli_ambiguous_cmd(char *cmd) {
	printf("%% Ambiguous command: \"%s\"\n", cmd);
}

void swcli_incomplete_cmd(char *cmd) {
	printf("%% Incomplete command.\n");
}

void swcli_unimplemented_cmd(char *cmd) {
	printf("%% Command not (yet) implemented.\n");
}

void swcli_extra_input(int off) {
	int i;
	for (i=0; i<off+strlen(prompt); i++) printf(" ");
	printf("^\n");
	printf("%% Invalid input detected at '^' marker.\n\n");
}

void swcli_go_ahead() {
	printf("  <cr>\n");
}

void init_exec_state(sw_execution_state_t *ex) {
	int i;

	ex->func = NULL;
	ex->pipe_output = 0;
	ex->pipe_type = 0;
	ex->runnable = 0;
	if (ex->func_args) {
		for (i=0; i<ex->num; i++)
			free(ex->func_args[i]);
		free(ex->func_args);
	}	
	ex->func_args = (char **) malloc(INITIAL_ARGS_NUM *sizeof(char *));
	ex->size = INITIAL_ARGS_NUM;
	ex->num = 0;
}

void dump_exec_state(sw_execution_state_t *ex) {
	int i;
	printf("\nExecution state dump: \n"
		"func: %p\n"
		"pipe_output: %d\n"
		"pipe_type: %d\n"
		"runnable: %d\n",
		ex->func, ex->pipe_output,
		ex->pipe_type, ex->runnable);
	printf("func_args: ");
	for (i=0; i<ex->num; i++)
		printf("%s ", ex->func_args[i]);
	printf("\n\n");
}

/* Help function called when the user
 * pressed '?' in the command line.
 */
int list_current_options(int something, int key) {
	int i = 0, count = 0, ret = 0, c;
	char *cmd, *lasttok;
	char *spec = strdup("%%-%ds ");
	char aspec[8];
	FILE *pipe;
	sw_match_t *matched;


	printf("%c\n", key);
	/* Establish a current search set based on the current line buffer */
	if ((ret = parse_command(strdup(rl_line_buffer), change_search_scope)) > 1) {
		swcli_ambiguous_cmd(rl_line_buffer);
		goto out;
	}
	if (!search_set && !handler) {
		swcli_invalid_cmd();
		goto out;
	}
	else if (!search_set && handler) {
		/* complete command with no search set */
		if (ret <= 0)
			swcli_invalid_cmd();
		else 
			swcli_go_ahead();
		goto out;
	}

	if (!strlen(rl_line_buffer) || rl_line_buffer[strlen(rl_line_buffer)-1] == ' ') {
		/* List all commands in current set with help message */
		/* output is piped to the unix command more */
		if (ret < 0){
			swcli_invalid_cmd();
			goto out;
		}
		rl_deprep_terminal();
		if (!(pipe=popen(PAGER_PATH, "w"))) {
			perror("popen");
			exit(1);
		}
		for (i=0; (cmd = search_set[i].name); i++) {
			if(search_set[i].priv > priv)
				continue;
			fprintf(pipe, "  %-20s\t%s\n", cmd, search_set[i].doc);
		}
		pclose(pipe);
		/* complete command with search set */
		if (handler) 
			swcli_go_ahead();
		rl_prep_terminal(1);
	}
	else {
		/* List possible completions from the current search set */
		for (lasttok=&rl_line_buffer[strlen(rl_line_buffer)-1]; 
				lasttok!=rl_line_buffer && !whitespace(*lasttok); lasttok--);
		if (lasttok != rl_line_buffer) lasttok++;

		matched = get_matches(&count, lasttok);
		for (i=0; i<count; i++) {
			if (i && !(i % MATCHES_PER_ROW))
				printf("\n");
			memset(aspec, 0, sizeof(aspec));
			c = sprintf(aspec, spec, matched[i].pwidth);
			assert(c < sizeof(aspec));
			printf(aspec, matched[i].text);
		}

		if (!count) {
			swcli_invalid_cmd();
			goto out;
		}
		else { 
			free(matched);
			printf("\n");
		}	
	}
	printf("\n");
		
out:	
	rl_forced_update_display();
	free(spec);
	return ret;
}

/* Hanlder that can be installed 
   on a key sequence */
int swcli_ignore_keyseq(int i, int j) {
	return 0;
}


/* Override some readline defaults */
int swcli_init_readline() {

	dbg("Init readline\n");
	/* Allow conditional parsing of ~/.inputrc file */
	rl_readline_name = "SwCli";

	/* Setup to ignore SIGINT and SIGTSTP */
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	/* Tell the completer we want a crack first */
	rl_attempted_completion_function = swcli_completion;
	/* FIXME: this doesn't work with older readline (readline-4.3-7) 
	   Used it sucessfuly on the latest readline (5.0)
	 */
	//rl_completion_word_break_hook = swcli_completion_word_break;
	rl_completer_word_break_characters = strdup(" ");
	rl_bind_key('?', list_current_options);
	rl_set_key("\\C-l", swcli_ignore_keyseq, rl_get_keymap());
	return 0;
}

/*
  Selects the current search command set 
  based on the value of match (current command token analysed)
  Used by the completion mechanism.
 */
int change_search_scope(char *match, char *rest, char lookahead)  {
	int i=0, count = 0;
	char *name, *arg;
	struct cmd *set = NULL;
	sw_command_handler func = NULL;

	if (!search_set) return 0;
    for (i=0; (name = search_set[i].name); i++) {
        if(search_set[i].priv > priv)
            continue;
		if (!strncmp(match,name,strlen(match)) && (whitespace(lookahead))) {
			count++;
			set = search_set[i].subcmd;
			func = search_set[i].func;
			if (!strcmp(match, name)) {
				count = 1;
				break;
			}
		}
		else if (search_set[i].valid) {
			arg = (search_set[i].state & PTCNT)? match: rest;
			if (search_set[i].valid(arg, lookahead)) {
				count = 1;
				if (search_set[i].state & PTCNT && whitespace(lookahead)) {
					func = search_set[i].func;
					set = search_set[i].subcmd;
				}	
				else  {
					set = search_set;
					func = handler;
				}	
				break;
			}	
		}
    }

	if (count==1) {
		search_set = set;
		handler = func;
		return count;
	}

	return count;
}

/*
  Selects the current command tree node 
  based on the value of match (current command token analysed)
  Used by the execution mechanism.
 */
int lookup_token(char *match, char *rest, char lookahead) {
	char *name, *arg;
	struct cmd *set = NULL;
	int i=0, count = 0;

	if (!search_set) {
		exec_state.runnable = NA;
		return 0;
	}
    for (i=0; (name = search_set[i].name); i++) {
		if (search_set[i].priv > priv)
			continue;
		if (search_set[i].valid) {
			arg = (search_set[i].state & PTCNT)? match: rest;
			if (search_set[i].valid(arg, lookahead)) {
				count = 1;
				if (search_set[i].state & PTCNT && whitespace(lookahead))
					set = search_set[i].subcmd;
				else 
					set = search_set;
				exec_state.runnable = search_set[i].state;
				if (!exec_state.pipe_output)
					exec_state.func = search_set[i].func;
				if (exec_state.num >= exec_state.size) {
					exec_state.func_args = realloc(exec_state.func_args,
							(exec_state.size + INITIAL_ARGS_NUM)*sizeof(char *));
					assert(exec_state.func_args);
					exec_state.size += INITIAL_ARGS_NUM;
				}
				exec_state.func_args[exec_state.num++] = strdup(arg);
				break;
			}
		}
		else if (!strncmp(match, name, strlen(match))) {
			count++;
			exec_state.runnable = search_set[i].state;
			if (!strcmp(match, "|"))
				exec_state.pipe_output = 1;
			if (!exec_state.pipe_output)	
				exec_state.func = search_set[i].func;
			else if (!(search_set[i].state & RUN)) {
				exec_state.runnable = 0;
				exec_state.pipe_type = search_set[i].state & MODE_MASK;
			}
			/* setup to advance */
			set = search_set[i].subcmd;
			if (!strcmp(match, name)) {
				count = 1;
				break;
			}
		}
	}

	if (count == 1) {
		search_set = set;
		return count;
	}
	
	return count;
}


/*
  Parses the command line buffer, splits it
  into tokens and invokes change_search_scope()
  on each token 
  */
int parse_command(char *line_buffer, int (* next_token)(char *, char *, char)) {
	char *start, *tmp, *rest;
	char c;
	int ret = 0;

	if (!line_buffer) return ret;
	dbg("\n");
	start = line_buffer;
	search_set = cmd_root->cmd;
	handler = NULL;
	do {
		tmp = start;
		while (whitespace(*tmp)) tmp++;	
		if (*tmp=='\0') break;
		start = tmp;
		rest = strdup(start);
		while (*tmp!='\0' && !whitespace(*tmp)) tmp++;
		c = *tmp;
		*tmp = '\0';
		/* if we didn't have a single match followed by whitespace 
		 then we must abandon right here */
		if ((ret = next_token(start, rest, c)) > 1) {
			*tmp = c;
			free(rest);
			break;
		}
		else if (!ret) {
			*tmp = c;
			ret = line_buffer - start;
			free(rest);
			break;
		}
		dbg("COMMAND TOKEN: '%s'\n", start);
		*tmp = c;
		if (c == '\0') break;
		start = tmp+1;
	} while (*start!='\0');

	free(line_buffer);
	return ret;
}

/* Override default readline behavior of printing
 * the list of matches on completion (if there is
 * more than one match).
 * We just go to the next line and do a redisplay 
 * of the command line.
 */
void display_matches_hook(char **matches, int i, int j) {
	printf("\n");
	rl_forced_update_display();
}

/* Attempt to complete on the contents of text. start and end
 * bound the region of rl_line_buffer that contains the word to
 * complete. TEXT is the word to complete. We can use the entire
 * contents of rl_line_buffer in case we want to do some simple
 * parsing. Return the array of matches, or NULL if there aren't any */
char ** swcli_completion(const char *text, int start, int end) {
	char **matches;

	matches = (char **)NULL;

	parse_command(strdup(rl_line_buffer), change_search_scope);

	rl_attempted_completion_over = 1;
 	rl_completion_display_matches_hook = display_matches_hook;
	matches = rl_completion_matches(text, swcli_generator);

	if (!matches) /* Force redisplay, even when we have no match */
		display_matches_hook(matches, start, end);
	return (matches);
}

/* Generator function for command completion. state lets us
 * know whether to start from scratch; whithout any state
 * (i.e. state == 0), then we start at the top of the list. */
char *swcli_generator(const char *text, int state) {
	static int list_index, len;
	char *name;

	dbg("generator: search_set at 0x%x\n", search_set);
	
	if (! search_set) 
		return ((char *)NULL);

	/* on first call do some initialization */
	if (!state) {
		list_index = 0;
		len = strlen(text);
	}


	/* Return the next name which partially matches from the
	 * command list */
	for (; (name = search_set[list_index].name); list_index++) {
        if(search_set[list_index].priv > priv)
            continue;
		if (strncmp(name, text, len) == 0 && 
				(search_set[list_index].state & CMPL || !search_set[list_index].valid)) {
			list_index++;
			return strdup(name);
		}
	}
	
	/* if no matches found then return NULL */
	return ((char *)NULL);
}

sw_match_t *get_matches(int *matched, char *token) {
	int i=0, count = 0, j, num;
	char *cmd;
	sw_match_t *matches;

	matches = (sw_match_t *) malloc(MATCHES_PER_ROW * sizeof(sw_match_t));
	num = MATCHES_PER_ROW;
	for (i=0; (cmd = search_set[i].name); i++) {
		if(search_set[i].priv > priv)
			continue;
		if (!strncmp(token, cmd, strlen(token)) || 
				(search_set[i].valid && search_set[i].valid(token, token[strlen(token)-1]))) {
			count++;
			if (count - 1 >= num) {
				num += MATCHES_PER_ROW;
				matches = (sw_match_t *) realloc(matches, num * sizeof(sw_match_t));
				assert(matches);
			}
			matches[count-1].text = cmd;
			matches[count-1].pwidth = strlen(cmd);
			if (count > MATCHES_PER_ROW) {
				/* update pwidths backward */
				j = count;
				while (j - MATCHES_PER_ROW - 1 >= 0) {
					if (matches[j - MATCHES_PER_ROW - 1].pwidth < strlen(cmd))
						matches[j - MATCHES_PER_ROW - 1].pwidth = strlen(cmd);
					j = j - MATCHES_PER_ROW;
				}
				
			}
		}
	}
	*matched = count;
	if (!count) {
		free(matches);
		matches = NULL;
	}
	return matches;
}

void swcli_piped_exec(sw_execution_state_t *exc) {
	int nc;
	char cmd_buf[MAX_LINE_WIDTH];
	FILE *pipe;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	if (!exc->pipe_output) {
		nc = sprintf(cmd_buf, "%s", PAGER_PATH);
	}
	else {
		/*
			FIXME: FIXME: FIXME: 
			Implementare escapeshellarg() pe cmd_buf
			De altfel daca nu facem asta merge super beton
			sa executi comenzi de shell intre `` !!!
		 */
		nc = sprintf(cmd_buf, "./filter %d %s", exc->pipe_type, exc->func_args[exc->num-1]);	
	}
	assert(nc < sizeof(cmd_buf));
	assert((pipe = popen(cmd_buf, "w")));
	if (exc->num >= exc->size) {
		exc->func_args = realloc(exc->func_args,
				(exc->size + 1) *sizeof(char *));
		assert(exc->func_args);
		exc->size ++;
	}
	exc->func_args[exc->num++] = NULL;
	exc->func(pipe, exc->func_args); //FIXME pointer la arg
	pclose(pipe);
}

void swcli_exec_cmd(char *cmd) {

	init_exec_state(&exec_state);
	int ret = parse_command(strdup(cmd), lookup_token);

//	dump_exec_state(&exec_state);
//	printf("ret: %d\n", ret);
	if (ret <= 0) {
		swcli_extra_input(abs(ret));
		return;
	}
	if (ret > 1) {
		swcli_ambiguous_cmd(cmd);
		return;
	}
	
	if(exec_state.runnable & NA) {
		swcli_ambiguous_cmd(cmd);
		return;
	}
	if(exec_state.runnable & RUN) {
		if (exec_state.func)
			swcli_piped_exec(&exec_state);
		else 
			swcli_unimplemented_cmd(cmd);
		return;
	}
	swcli_incomplete_cmd(cmd);
}

int climain(void) {
	char hostname[MAX_HOSTNAME];
	char *cmd = NULL;
	HIST_ENTRY *pentry;
	int status;

	/* initialization */
	sprintf(eth_range, "<0-7>"); /* FIXME luate dinamic */
	status = cfg_init();
	assert(!status);
	swcli_init_readline();
	sock_fd = socket(PF_PACKET, SOCK_RAW, 0);
	if(sock_fd == -1) {
		perror("socket");
		return 1;
	}
	
	do {
		if (cmd) {
			free(cmd);
			cmd = (char *)NULL;
		}

		gethostname(hostname, sizeof(hostname));
		hostname[sizeof(hostname) - 1] = '\0';
		sprintf(prompt, cmd_root->prompt, hostname, priv > 1 ? '#' : '>');
		search_set = cmd_root->cmd;

		cmd = readline(prompt);
		if (!cmd) {
			 /* Avoid exiting on EOF */
			 cmd = strdup("");
			 rl_crlf();
			 continue;
		}
		if (cmd && *cmd) {
			dbg("Command was: '%s'\n", cmd);
			if (history_length) {
				pentry = history_get(history_length);
				if (pentry && strcmp(pentry->line, cmd))
					add_history(cmd);
			}
			else 
				add_history(cmd);
			swcli_exec_cmd(cmd);
		}
	} while (cmd);
	
	/* cleanup */
	close(sock_fd);

	return 0;
}

FILE *mk_tmp_stream(char *name, char *mode) {
	char tmp_name[] = "/tmp/swcli.XXXXXX\0";
	int tmp_fd;

	tmp_fd = mkstemp(tmp_name);
	if(tmp_fd == -1)
		return NULL;
	if(name != NULL)
		strcpy(name, tmp_name);
	return fdopen(tmp_fd, mode);
}

void copy_data(FILE *dst, FILE *src) {
	unsigned char buf[4096];
	size_t nmemb;

	while((nmemb = fread(buf, 1, sizeof(buf), src)))
		fwrite(buf, nmemb, 1, dst);
}

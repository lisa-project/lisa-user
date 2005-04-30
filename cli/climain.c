#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

#include "debug.h"
#include "climain.h"
#include "command.h"

sw_command_root_t *cmd_root = &command_root_main;
static sw_command_t *search_set;
char prompt[MAX_HOSTNAME + 32];
static rl_icpfunc_t *handler = NULL;

/* Current privilege level */
static int priv = 0;

/* Error reporting & stuff */
void swcli_invalid_cmd() {
	printf("%% Unrecognized command\n");
}

void swcli_ambiguous_cmd(char *cmd) {
	printf("%% Ambiguous command: \"%s\"\n", cmd);
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
	if ((ret = select_search_scope(strdup(rl_line_buffer), 0)) > 1) {
		swcli_ambiguous_cmd(rl_line_buffer);
		goto out;
	}
	if (!search_set && !handler) {
		swcli_invalid_cmd();
		goto out;
	}
	else if (!search_set && handler) {
		/* complete command with no search set */
		if (!ret)
			swcli_invalid_cmd();
		else 
			swcli_go_ahead();
		goto out;
	}

	if (!strlen(rl_line_buffer) || rl_line_buffer[strlen(rl_line_buffer)-1] == ' ') {
		/* List all commands in current set with help message */
		/* output is piped to the unix command more */
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
		/* complete command with search set */
		if (handler) 
			swcli_go_ahead();
//		for (i=0; i<100; i++) fprintf(pipe, "More functionality ;)\n");
		pclose(pipe);
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

/* Override some readline defaults */
int swcli_init_readline() {

	dbg("Init readline\n");
	/* Allow conditional parsing of ~/.inputrc file */
	rl_readline_name = "SwCli";

	/* Tell the completer we want a crack first */
	rl_attempted_completion_function = swcli_completion;
	rl_bind_key('?', list_current_options); 
	return 0;
}

/*
  Selects the current search command set 
  based on the value of match (current command token analysed)
 */
int change_search_scope(char *match, char lookahead)  {
	int i=0, count = 0;
	char *name;
	struct cmd *set = NULL;
	rl_icpfunc_t *func = NULL;

	dbg("change_search_scope(%s): search_set at 0x%x\n", match, search_set);
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
    }

	if (count==1) {
		search_set = set;
		handler = func;
		return count;
	}

	return count;
}

/*
  Parses the command line buffer, splits it
  into tokens and invokes change_search_scope()
  on each token 
  */
int select_search_scope(char *line_buffer, char exec) {
	char *start, *tmp;
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
		while (*tmp!='\0' && !whitespace(*tmp)) tmp++;
		c = *tmp;
		*tmp = '\0';
		/* if we didn't have a single match followed by whitespace 
		 then we must abandon right here */
		if ((ret = change_search_scope(start, exec?exec:c)) > 1) {
			*tmp = c;
			break;
		}
		else if (!ret && exec) {
			*tmp = c;
			ret = line_buffer - start;
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
	
	select_search_scope(strdup(rl_line_buffer), 0);

	rl_attempted_completion_over = 1;
 	rl_completion_display_matches_hook = display_matches_hook;
	matches = rl_completion_matches(text, swcli_generator);

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
		if (strncmp(name, text, len) == 0) {
			dbg("match: %s\n", name);
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
		if (!strncmp(token, cmd, strlen(token))) {
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

void swcli_exec_cmd(char *cmd) {
	int ret = select_search_scope(strdup(cmd), ' ');

	if (ret > 1) {
		swcli_ambiguous_cmd(cmd);
		return;
	}
	if (ret <= 0) {
		swcli_extra_input(abs(ret));
		return;
	}
	
	if (!search_set && handler) {
		/* complete command */
		handler(NULL);
	}
	else {
		swcli_invalid_cmd();
	}
}

int climain(void) {
	char hostname[MAX_HOSTNAME];
	char *cmd = NULL;

	/* initialization */
	swcli_init_readline();
	
	do {
		if (cmd) {
			free(cmd);
			cmd = (char *)NULL;
		}

		gethostname(hostname, sizeof(hostname));
		hostname[sizeof(hostname) - 1] = '\0';
		sprintf(prompt, cmd_root->prompt, hostname, priv ? '#' : '>');
		search_set = cmd_root->cmd;

		cmd = readline(prompt);
		if (cmd && *cmd) {
			dbg("Command was: '%s'\n", cmd);
			swcli_exec_cmd(cmd);
			add_history(cmd);
		}
	} while (cmd);
	
	return 0;
}


/* Command Handlers implementation */
int cmd_disable(char *arg) {
	priv = 0;
	return 0;
}

int cmd_enable(char *arg) {
	priv = 1;
	return 0;
}

int cmd_exit(char *arg) {
	exit(0);
}

int cmd_conf_t(char *arg) {
	cmd_root = &command_root_config;
	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	/* FIXME binding readline pentru ^Z */
	return 0;
}

int cmd_help(char *arg) {
	printf(
		"Help may be requested at any point in a command by entering\n"
		"a question mark '?'.  If nothing matches, the help list will\n"
		"be empty and you must backup until entering a '?' shows the\n"
		"available options.\n"
		"Two styles of help are provided:\n"
		"1. Full help is available when you are ready to enter a\n"
		"   command argument (e.g. 'show ?') and describes each possible\n"
		"   argument.\n"
		"2. Partial help is provided when an abbreviated argument is entered\n"
		"   and you want to know what arguments match the input\n"
		"   (e.g. 'show pr?'.)\n\n"
		);
	return 0;
}

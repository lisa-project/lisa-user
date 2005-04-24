#include <assert.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

#include "debug.h"
#include "climain.h"
#include "command.h"


static sw_command_root_t *current_root = command_root;
static sw_command_t *search_set;
/* Initialization moved to main()   = current_root->cmd; */
/* Current privilege level */
static int priv = 0;

void swcli_invalid_cmd() {
	printf("% Unrecognized command\n");
}

int list_current_options(int something, int key) {
	int i = 0, count = 0, c;
	char *cmd, *lasttok;
	char *spec = strdup("%%-%ds ");
	char aspec[8];
	FILE *pipe;
	sw_match_t *matched;


	printf("%c\n", key);
	/* Establish a current search set based on the current line buffer */
	select_search_scope(strdup(rl_line_buffer));
	if (!search_set) {
		swcli_invalid_cmd();
	}
	else {
		if (!strlen(rl_line_buffer) || rl_line_buffer[strlen(rl_line_buffer)-1] == ' ') {
			/* List all commands in current set with help message */
			/* output is piped to the unix command more */
			if (!(pipe=popen("/bin/more", "w"))) {
				perror("popen");
				exit(1);
			}
			while (cmd = search_set[i].name) {
				if(search_set[i].priv > priv)
					continue;
				fprintf(pipe, "  %-20s\t%s\n", cmd, search_set[i].doc);
				i++;
			}
//			for (i=0; i<100; i++) fprintf(pipe, "More functionality ;)\n");
			pclose(pipe);
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
	}	

out:	
	rl_forced_update_display();
	free(spec);
	return 0;
}

/* Override some readline defaults */
int swcli_init_readline() {

	dbg("Init readline\n");
	/* Allow conditional parsing of ~/.inputrc file */
	rl_readline_name = "SwCli";

	/* Tell the completer we want a crack first */
	rl_attempted_completion_function = swcli_completion;
	rl_bind_key('?', list_current_options); 
}

/*
  Selects the current search command set 
  based on the value of match (current command token analysed)
 */
int change_search_scope(char *match, char lookahead)  {
	int i=0, count = 0;
	int flag = 0;
	char *name;
	struct cmd *set = NULL;

	dbg("change_search_scope(%s): search_set at 0x%x\n", match, search_set);
	if (!search_set) return;
    while (name = search_set[i].name) {
        if(search_set[i].priv > priv)
            continue;
		if (!strcmp(match,name) && (whitespace(lookahead))) {
				search_set = search_set[i].subcmd;
				return 0;
		}
        i++;
    }

	return 1;
}

/*
  Parses the command line buffer, splits it
  into tokens and invokes change_search_scope()
  on each token 
  */
void select_search_scope(char *line_buffer) {
	char *start, *tmp;
	char c;

	if (!line_buffer) return;
	dbg("\n");
	start = line_buffer;
	/* FIXME FIXME FIXME (I may be enabled or in conf-t) */
	search_set = shell_main;
	do {
		tmp = start;
		while (whitespace(*tmp)) tmp++;	
		if (*tmp=='\0') break;
		start = tmp;
		while (*tmp!='\0' && !whitespace(*tmp)) tmp++;
		c = *tmp;
		*tmp = '\0';
		/* if we didn't have exact matched followed by whitespace 
		 then we must abandon right here */
		if (change_search_scope(start, c)) {
			*tmp = c;
			break;
		}
		dbg("COMMAND TOKEN: '%s'\n", start);
		*tmp = c;
		if (c == '\0') break;
		start = tmp+1;
	} while (*start!='\0');

	free(line_buffer);
}

void display_matches_hook(char **matches, int i, int j) {
	printf("\n");
	rl_forced_update_display();
}

/* Attempt to complete on the contents of TEXT. START and END
 * bound the region of rl_line_buffer that contains the word to
 * complete. TEXT is the word to complete. We can use the entire
 * contents of rl_line_buffer in case we want to do some simple
 * parsing. Return the array of matches, or NULL if there aren't any */
char ** swcli_completion(const char *text, int start, int end) {
	char **matches;

	matches = (char **)NULL;
	
	select_search_scope(strdup(rl_line_buffer));

	rl_attempted_completion_over = 1;
 	rl_completion_display_matches_hook = display_matches_hook;
	/*
	  FIXME: override rl_completion_matches to obtain 
	  cisco ios functionality 
	 */
	matches = rl_completion_matches(text, swcli_generator);

	return (matches);
}

/* Generator function for command completion. STATE lets us
 * know whether to start from scratch; whithout any state
 * (i.e. STATE == 0), then we start at the top of the list. */
char *swcli_generator(const char *text, int state) {
	static int list_index, len;
	char *name, *temp;

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
	while (name = search_set[list_index].name) {
		list_index++;
        if(search_set[list_index].priv > priv)
            continue;
		
		if (strncmp(name, text, len) == 0) {
			dbg("match: %s\n", name);
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
	while (cmd = search_set[i].name) {
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
		i++;
	}
	*matched = count;
	if (!count) {
		free(matches);
		matches = NULL;
	}
	return matches;
}

int climain(void) {
	char hostname[MAX_HOSTNAME];
	char prompt[MAX_HOSTNAME + 32];
	char *cmd = NULL;

	/* initialization */
	swcli_init_readline();
	search_set = current_root->cmd;
	
	do {
		/* Do this on every command because hostname may change. */
		gethostname(hostname, sizeof(hostname));
		hostname[sizeof(hostname) - 1] = '\0';
		sprintf(prompt, current_root->prompt, hostname, priv ? '#' : '>');

		if (cmd) {
			free(cmd);
			cmd = (char *)NULL;
		}
		cmd = readline(prompt);
		if (cmd && *cmd) {
			dbg("Command was: '%s'\n", cmd);
			add_history(cmd);
		}
	} while (cmd);
	
	return 0;
}


/* Command Handlers implementation */
int cmd_enable(char *arg) {
	printf("This the enable handler\n\n");
	return 0;
}

int cmd_exit(char *arg) {
	printf("Bye-bye\n");
	exit(0);
	return 0;
}

int cmd_help(char *arg) {
	printf("This is the application's help message\n\n");
	return 0;
}

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "debug.h"
#include "climain.h"
#include "command.h"


static struct cmd *search_set = shell_main;
/* Current privilege level */
static int priv = 0;

void swcli_invalid_cmd() {
	printf("% No match\n");
}

int list_current_options(int something, int key) {
	int i = 0;
	char *cmd, *lasttok;

	printf("%c\n", key);
	/* Establish a current search set based on the current 
	  line buffer */
	select_search_scope(strdup(rl_line_buffer));
	if (!search_set) {
		swcli_invalid_cmd();
	}
	else {
		if (!strlen(rl_line_buffer) || rl_line_buffer[strlen(rl_line_buffer)-1] == ' ') {
			/* List all commands in current set with help message */
			while (cmd = search_set[i].name) {
                if(search_set[i].priv > priv)
                    continue;
				printf("  %-20s\t%s\n", cmd, search_set[i].doc);
				i++;
			}
		}
		else {
			/* List possible completions from the current search set */
			for (lasttok=&rl_line_buffer[strlen(rl_line_buffer)-1]; 
					lasttok!=rl_line_buffer && !whitespace(*lasttok); lasttok--);
			if (lasttok != rl_line_buffer) lasttok++;
			while (cmd = search_set[i].name) {
                if(search_set[i].priv > priv)
                    continue;
				if (!strncmp(lasttok, cmd, strlen(lasttok))) {
					printf("%s  ", cmd);
				}
				i++;
			}
			printf("\n");
		}
		printf("\n");
	}	
	rl_forced_update_display();
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
void change_search_scope(char *match)  {
	int i=0;
	char *name;

	dbg("change_search_scope(%s): search_set at 0x%x\n", match, search_set);
	if (!search_set) return;
    while (name = search_set[i].name) {
        if(search_set[i].priv > priv)
            continue;
        if (!strcmp(match, name)) {
            search_set = search_set[i].subcmd;
            dbg("chage_search_scope(%s): new_search_set at 0x%x (pos=%d)\n", match, search_set, i);
            break;
        }
        i++;
    }
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
		change_search_scope(start);
		dbg("COMMAND TOKEN: '%s'\n", start);
		*tmp = c;
		if (c == '\0') break;
		start = tmp+1;
	} while (*start!='\0');

	free(line_buffer);
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

int climain(void) {
	char hostname[MAX_HOSTNAME];
	char prompt[MAX_HOSTNAME + 32];
	char *cmd = NULL;

	/* initialization */
	swcli_init_readline();
	
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	sprintf(prompt, "%s>", hostname);

	do {
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

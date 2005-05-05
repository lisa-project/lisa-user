#ifndef _CLIMAIN_H
#define _CLIMAIN_H

#define MAX_HOSTNAME 32
#define MATCHES_PER_ROW 5

#define PAGER_PATH "/bin/more"

#include "command.h"

typedef struct match {
	char *text;			/* Matched text */
	int pwidth;			/* Printing width */
} sw_match_t;

typedef struct execution {
	sw_command_handler func;
	int pipe_output;
	int pipe_type;
	int runnable;
	char *func_args;
} sw_execution_state_t;

extern sw_command_root_t *cmd_root;
extern int climain(void);

/* Command handlers */
extern void cmd_disable				__P((FILE *, char *));
extern void cmd_enable				__P((FILE *, char *));
extern void cmd_help				__P((FILE *, char *));
extern void cmd_exit				__P((FILE *, char *));
extern void cmd_conf_t				__P((FILE *, char *));
extern void cmd_history				__P((FILE *, char *));

/* Validation handlers */
extern int valid_regex				__P((char *));

/* Misc functions */
extern char *swcli_generator __P((const char *, int));
extern char **swcli_completion __P((const char *, int, int));
extern int parse_command(char *, int (*)(char *, char *, char));
extern int change_search_scope(char *, char *, char);
extern sw_match_t *get_matches(int *, char *);

extern char eth_range[];

#endif

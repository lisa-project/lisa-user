#ifndef _CLIMAIN_H
#define _CLIMAIN_H

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

#define MAX_HOSTNAME 32
#define MATCHES_PER_ROW 5
#define INITIAL_ARGS_NUM 8

#define PAGER_PATH "/bin/more"
#ifndef FILTER_PATH
#define FILTER_PATH "/bin/filter"
#endif

#include "command.h"
#include "shared.h"

typedef struct match {
	char *text;			/* Matched text */
	int pwidth;			/* Printing width */
} sw_match_t;

typedef struct execution {
	sw_command_handler func;
	int pipe_output;
	int pipe_type;
	int runnable;
	int size;
	int num;
	char **func_args;
} sw_execution_state_t;

typedef struct completion {
	sw_command_t *search_set;	/* Current command tree search set */
	sw_validation_func valid;	/* Validation function associated */
	char *cmd_full_name;
	int runnable;				/* runnable state */
	char *start;
	int offset;
	int cmpl;
	int no_match_token;
} sw_completion_state_t;

extern sw_command_root_t *cmd_root;
extern int climain(void);

/* Misc functions */
extern char *swcli_generator __P((const char *, int));
extern char **swcli_completion __P((const char *, int, int));
extern int parse_command(char *, int (*)(char *, char *, char));
extern int change_search_scope(char *, char *, char);
extern sw_match_t *get_matches(int *, char *);
extern void init_exec_state(sw_execution_state_t *ex);
extern int lookup_token(char *match, char *rest, char lookahead); 

extern int sock_fd;
extern int priv;
extern char prompt[];
extern FILE *mk_tmp_stream(char *, char *);
extern void copy_data(FILE *, FILE *);

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')

#endif

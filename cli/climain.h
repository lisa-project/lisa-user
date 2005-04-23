#ifndef _CLIMAIN_H
#define _CLIMAIN_H

#define MAX_HOSTNAME 32

extern int climain(void);

int cmd_enable __P((char *));
int cmd_help __P((char *));
int cmd_exit __P((char *));
char *swcli_generator __P((const char *, int));
char **swcli_completion __P((const char *, int, int));
void select_search_scope(char *);

typedef struct cmd {
	char *name;				/* User printable name of the function */
	rl_icpfunc_t *func;		/* Function call to do the job */
	char *doc;				/* Documentation for this function */
	struct cmd *subcmd;		/* Sub-commands */
} SW_COMMAND_T;

#endif

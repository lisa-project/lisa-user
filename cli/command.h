#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "filter.h"

/* Command node state */
enum {
	NA = MODE_GREP+1,
	INCOMPLETE,
	RUNNABLE
};

typedef void (*sw_command_handler)(FILE *, char *);

typedef struct cmd {
	char *name;					/* User printable name of the function */
    int priv;               	/* Minimum privilege level to execute this */
	rl_icpfunc_t *valid;		/* Function to validate pattern-matching args */
	sw_command_handler func;	/* Function call to do the job */
	int state;					/* Node state (runnable / incomplete) */
	char *doc;					/* Documentation for this function */
	struct cmd *subcmd;			/* Sub-commands */
} sw_command_t;

typedef struct root {
    char *prompt;
    sw_command_t *cmd;
} sw_command_root_t;

extern void cmd_help				__P((FILE *, char *));

extern char eth_range[];
extern char vlan_range[];

extern int valid_eth(char *);
extern int valid_vlan(char *);
extern int parse_eth(char *);
extern int parse_vlan(char *);

extern sw_command_root_t command_root_main;

#endif

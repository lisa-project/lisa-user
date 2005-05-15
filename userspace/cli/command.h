#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdio.h>
#include <readline/readline.h>
/* FIXME: cu readline 5.0 nu se compileaza cu include-ul de jos */
#include <readline/history.h>

#include "filter.h"

/* Command node state */
/* Node is runnable (command can be executed at this point) */
#define RUN			0x0010
/* Node is a pattern that ends at the first space */
#define PTCNT		0x0020
/* Command is ambiguous */
#define NA			0x0040
/* Node is a pattern that needs completion */
#define CMPL		0x0080
#define FLAGS_MASK	0x00f0

typedef void (*sw_command_handler)(FILE *, char **);

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

extern void cmd_sh_int				__P((FILE *, char **));
extern void cmd_help				__P((FILE *, char **));

extern char eth_range[];
extern char vlan_range[];

extern int valid_eth(char *);
extern int valid_vlan(char *);
extern int parse_eth(char *);
extern char *if_name_eth(char *);
extern int parse_vlan(char *);
extern int valid_mac(char *);
extern int parse_mac(char *, unsigned char *);

extern sw_command_root_t command_root_main;

#endif

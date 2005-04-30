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

extern sw_command_root_t *cmd_root;
extern int climain(void);

extern int cmd_disable				__P((char *));
extern int cmd_enable				__P((char *));
extern int cmd_help					__P((char *));
extern int cmd_exit					__P((char *));
extern int cmd_conf_t				__P((char *));

extern char *swcli_generator __P((const char *, int));
extern char **swcli_completion __P((const char *, int, int));
extern int select_search_scope(char *, char);
extern sw_match_t *get_matches(int *, char *);

#endif

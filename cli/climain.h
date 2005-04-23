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

#endif

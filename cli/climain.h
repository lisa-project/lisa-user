#ifndef _CLIMAIN_H
#define _CLIMAIN_H

#define MAX_HOSTNAME 32
#define MATCHES_PER_ROW 5

typedef struct match {
	char *text;			/* Matched text */
	int pwidth;			/* Printing width */
} sw_match_t;

extern int climain(void);

int cmd_enable __P((char *));
int cmd_help __P((char *));
int cmd_exit __P((char *));
char *swcli_generator __P((const char *, int));
char **swcli_completion __P((const char *, int, int));
void select_search_scope(char *);
sw_match_t *get_matches(int *, char *);

#endif

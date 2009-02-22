#ifndef _CDP_H
#define _CDP_H

/* Prototypes for cdp related command handlers */
int cmd_sh_cdp(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_entry(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_holdtime(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne_detail(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_timer(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_traffic(struct cli_context *, int, char **, struct menu_node **);

#endif

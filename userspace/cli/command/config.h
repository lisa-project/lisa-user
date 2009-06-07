#ifndef _CONFIG_H
#define _CONFIG_H

int cmd_cdp_v2(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_holdtime(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_timer(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_setenpw(struct cli_context *, int, char **, struct menu_node **);
int cmd_setenpw_encrypted(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);
int cmd_hostname(struct cli_context *, int, char **, struct menu_node **);
int cmd_int_any(struct cli_context *, int, char **, struct menu_node **);
int cmd_linevty(struct cli_context *, int, char **, struct menu_node **);
int cmd_set_aging(struct cli_context *, int, char **, struct menu_node **);
int cmd_macstatic(struct cli_context *, int, char **, struct menu_node **);
int cmd_noensecret(struct cli_context *, int, char **, struct menu_node **);
int cmd_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_add_mrouter(struct cli_context *, int, char **, struct menu_node **);

#endif

#ifndef _CONFIG_LINE_H
#define _CONFIG_LINE_H

int cmd_setpw(struct cli_context *, int, char **, struct menu_node **);
int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);

#endif

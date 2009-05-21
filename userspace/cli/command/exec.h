#ifndef _EXEC_H
#define _EXEC_H

int swcli_output_modifiers_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev);

int cmd_conf_t(struct cli_context *, int, char **, struct menu_node **);
int cmd_disable(struct cli_context *, int, char **, struct menu_node **);
int cmd_enable(struct cli_context *, int, char **, struct menu_node **);
int cmd_quit(struct cli_context *, int, char **, struct menu_node **);
int cmd_help(struct cli_context *, int, char **, struct menu_node **);
int cmd_ping(struct cli_context *, int, char **, struct menu_node **);
int cmd_reload(struct cli_context *, int, char **, struct menu_node **);
int cmd_history(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_clock(struct cli_context *, int , char **, struct menu_node **);
int cmd_sh_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_ip(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_ip_igmps(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_ip_igmps_mrouter(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_ip_igmps_groups(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_addr(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_mac_age(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_priv(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_start(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_ver(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_trace(struct cli_context *, int, char **, struct menu_node **);

int cmd_sh_mac_addr_t(struct cli_context *, int, char **, struct menu_node **);
int cmd_cl_mac_addr_t(struct cli_context *, int, char **, struct menu_node **);

#endif

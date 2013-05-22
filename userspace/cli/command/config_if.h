#ifndef _CMD_CONFIG_IF_H
#define _CMD_CONFIG_IF_H

enum {
	CMD_VLAN_SET,
	CMD_VLAN_ADD,
	CMD_VLAN_ALL,
	CMD_VLAN_EXCEPT,
	CMD_VLAN_NONE,
	CMD_VLAN_REMOVE,
	CMD_VLAN_NO
};

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')

int cmd_rstp_if_set(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_if_set(struct cli_context *, int, char **, struct menu_node **);
int cmd_if_desc(struct cli_context *, int, char **, struct menu_node **);
int cmd_speed_duplex(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);
int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
int cmd_help(struct cli_context *, int, char **, struct menu_node **);
int cmd_swport(struct cli_context *, int, char **, struct menu_node **);
int cmd_noacc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_trunk_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_nomode(struct cli_context *, int, char **, struct menu_node **);
int cmd_setmode(struct cli_context *, int, char **, struct menu_node **);
int cmd_shutdown(struct cli_context *, int, char **, struct menu_node **);
int cmd_acc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_access(struct cli_context *, int, char **, struct menu_node **);
int cmd_trunk(struct cli_context *, int, char **, struct menu_node **);
int cmd_ip(struct cli_context *, int, char **, struct menu_node **);

#endif

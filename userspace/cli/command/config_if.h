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

int cmd_cdp_if_set(struct cli_context *, int, char **, struct menu_node **);
int cmd_if_desc(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_auto(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_full(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_half(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);
int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
int cmd_help(struct cli_context *, int, char **, struct menu_node **);
int cmd_noshutd(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_auto(struct cli_context *, int, char **, struct menu_node **);
int cmd_swport_off(struct cli_context *, int, char **, struct menu_node **);
int cmd_noacc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_trunk_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_nomode(struct cli_context *, int, char **, struct menu_node **);
int cmd_shutd(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_10(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_100(struct cli_context *, int, char **, struct menu_node **);
int cmd_swport_on(struct cli_context *, int, char **, struct menu_node **);
int cmd_acc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_access(struct cli_context *, int, char **, struct menu_node **);
int cmd_trunk(struct cli_context *, int, char **, struct menu_node **);
int cmd_ip(struct cli_context *, int, char **, struct menu_node **);

#endif

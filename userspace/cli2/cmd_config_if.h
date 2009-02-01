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

int cmd_trunk_vlan(struct cli_context *, int, char **, struct menu_node **);

#endif

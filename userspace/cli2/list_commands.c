#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include "swsock.h"

#include "cli.h"
#include "swcli_common.h"
#include "menu_interface.h"

#define MAX_DEPTH 32

extern struct menu_node menu_main;
extern struct menu_node config_main;
extern struct menu_node config_if_main;
extern struct menu_node config_vlan_main;
extern struct menu_node config_line_main;

const char *stack[MAX_DEPTH];

void dump_cmd(struct menu_node *node, int lev) {
	struct menu_node **np;
	int i;

	assert(lev < MAX_DEPTH);
	stack[lev] = node->name;

	if (!lev)
		printf(node->name ? "(%s)#\n" : "#\n", node->name);

	if (node->run) {
		for (i = 1; i <= lev; i++)
			printf("%s ", stack[i]);
		printf("\n");
	}

	if (node->subtree == NULL)
		return;
	
	for (np = node->subtree; *np; np++) {
		if (!strcmp((*np)->name, "|"))
			continue;
		dump_cmd(*np, lev + 1);
	}

	if (!lev)
		printf("\n");
}

int main(int argc, char **argv) {
	dump_cmd(&menu_main, 0);
	dump_cmd(&config_main, 0);
	dump_cmd(&config_if_main, 0);
	dump_cmd(&config_vlan_main, 0);
	dump_cmd(&config_line_main, 0);

	return 0;
}

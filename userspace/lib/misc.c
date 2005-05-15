#include <linux/net_switch.h>
#include <stdio.h>
#include <stdlib.h>

void cmd_showmac(FILE *out, char *arg)  {
	struct net_switch_mac *mac;
	struct net_switch_ioctl_arg *user_arg = (struct net_switch_ioctl_arg *)arg;
	int size = 0, actual_size = user_arg->ext.marg.actual_size;
	char *buf = user_arg->ext.marg.buf;

	
	fprintf(out, "Destination Address  Address Type  VLAN  Destination Port\n"
			"-------------------  ------------  ----  ----------------\n");
	while (size < actual_size) {
		mac = (struct net_switch_mac *)(buf + size);
		fprintf(out, "%02x%02x.%02x%02x.%02x%02x       "
				"%12s  %4d  %s\n", 
				mac->addr[0], mac->addr[1], mac->addr[2],
				mac->addr[3], mac->addr[4], mac->addr[5],
			    (mac->addr_type)? "Static" : "Dynamic",
				mac->vlan,
				mac->port
				);
		size += sizeof(struct net_switch_mac);
	}
}

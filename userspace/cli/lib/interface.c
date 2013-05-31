#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/netdevice.h>

#include "swsock.h"
#include "interface.h"

int if_tok_if(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out)
{
	int ret = cli_tokenize(ctx, buf, tree, out);

	if (out->partial_match && out->partial_match->priv) {
		out->matches[0] = out->partial_match;
		out->matches[1] = NULL;

		out->partial_match = NULL;
		out->len = out->ok_len;
		ret = 1;
	}

	return ret;
}

int if_parse_args(char **argv, struct menu_node **nodev, char *name, int *n)
{
	int __n, ret;

	do {
		if (!strcmp(nodev[0]->name, "Ethernet")) {
			__n = if_name_ethernet(name, argv[1]);
			ret = IF_T_ETHERNET;
			break;
		}

		if (!strcmp(nodev[0]->name, "vlan")) {
			__n = if_name_vlan(name, argv[1]);
			ret = IF_T_VLAN;
			break;
		}

		if (strcmp(nodev[0]->name, "netdev")) {
			__n = 0;
			ret = IF_T_ERROR;
			break;
		}

		__n = -1;

		if (strlen(argv[1]) >= IFNAMSIZ) {
			ret = IF_T_ERROR;
			break;
		}

		strcpy(name, argv[1]);
		ret = IF_T_NETDEV;
	} while (0);

	if (n)
		*n = __n;

	return ret;
}

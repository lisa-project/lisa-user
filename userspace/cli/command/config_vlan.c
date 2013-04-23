#include "swcli.h"
#include "config_vlan.h"

int cmd_namevlan(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct swcli_context *uc = SWCLI_CTX(ctx);
	int status;

	assert(argc);

	if (strcmp(nodev[0]->name, "no")) {
		/* vlan is renamed by user */
		assert(argc >= 2);
		status = switch_set_vlan_desc(uc->vlan, argv[1]);
	} else {
		/* vlan name is set to default */
		status = switch_set_vlan_desc(uc->vlan, NULL);
	}
	if (status) {
		EX_STATUS_REASON(ctx, "Could not name vlan %d\n", uc->vlan);
		return CLI_EX_REJECTED;
	}

	return CLI_EX_OK;
}

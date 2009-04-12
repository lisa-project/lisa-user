#include "swcli.h"
#include "config_line.h"

int cmd_setpw(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	swcli_dump_args(ctx, argc, argv, nodev);
	return 0;
}

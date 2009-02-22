#include "common.h"
#include "cdp_client.h"

int cmd_sh_cdp(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	return 0;
}

int cmd_sh_cdp_entry(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_ne(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){
	dump_args(ctx, argc, argv, nodev);
	return 0;
}
int cmd_sh_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_traffic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

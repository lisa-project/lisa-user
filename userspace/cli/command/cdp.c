#include "swcli.h"
#include "cdp_client.h"

int cmd_sh_cdp(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	FILE *out;

	shared_get_cdp(&cdp);
	if (cdp.enabled) {
		out =  ctx->out_open(ctx, 0);
		fprintf(out, "Global CDP information:\n"
				"\tSending CDP packets every %d seconds\n"
				"\tSending a holdtime value of %d seconds\n"
				"\tSending CDPv2 advertisements is %s\n",
				cdp.timer, cdp.holdtime, cdp.version==2? "enabled" : "disabled");
		fclose(out);
	}

	return 0;
}

int cmd_sh_cdp_entry(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	dump_args(ctx, argc, argv, nodev);
	return 0;
}


int cmd_sh_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_ne(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){
	dump_args(ctx, argc, argv, nodev);
	return 0;
}
int cmd_sh_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}
int cmd_sh_cdp_traffic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev){return 0;}

#include "rlshell.h"
#include "swcli_common.h"
#include "menu_main.h"

int main(int argc, char **argv) {
	struct rlshell_context ctx;
	struct swcli_context uc;

	ctx.cc.node_filter = PRIV_FILTER(15);
	ctx.cc.root = &menu_main;
	ctx.cc.out_open = cli_out_open;
	ctx.prompt = swcli_prompt;
	ctx.exit = 0;
	ctx.enable_ctrl_z = 0;
	ctx.uc = &uc;
	ctx.sock_fd = -1;

	rlshell_main(&ctx);

	return 0;
}

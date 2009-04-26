#include "swcli.h"

extern struct menu_node menu_main;

int main(int argc, char **argv) {
	struct swcli_context ctx;

	CLI_CTX(&ctx)->node_filter = PRIV_FILTER(15);
	CLI_CTX(&ctx)->root = &menu_main;
	CLI_CTX(&ctx)->out_open = cli_out_open;
	RLSHELL_CTX(&ctx)->prompt = swcli_prompt;
	RLSHELL_CTX(&ctx)->exit = 0;
	RLSHELL_CTX(&ctx)->enable_ctrl_z = 0;
	ctx.sock_fd = -1;
	ctx.cdp = NULL;

	if (shared_init() < 0) {
		perror("shared_init");
		return -1;
	}

	rlshell_main(RLSHELL_CTX(&ctx));

	return 0;
}

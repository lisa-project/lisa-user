/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>

#include "swcli.h"

extern struct menu_node menu_main;

static void redisplay_void(void) {}

static int waitcr(void)
{
	rl_voidfunc_t *old_redisplay = rl_redisplay_function;

	rl_redisplay_function = redisplay_void;
	readline(NULL);
	rl_redisplay_function = old_redisplay;
	return 0;
}

int main(int argc, char *argv[])
{
	struct swcli_context ctx;
	char hostname[HOST_NAME_MAX];
	
	if (switch_init() < 0) {
		perror("switch_init");
		return -1;
	}

	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	printf(
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"
			"\r\n\r\n%s con0 is now available\r\n\r\n\r\n\r\n\r\n\r\n"
			"Press RETURN to get started."
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n",
			hostname);
	waitcr();

	/* FIXME: enable should behave differently when the cli is invoked from
	 * console, i.e. it should not warn the user that no password is set.
	 * In the old cli this was done by setting a global flag:
	 * console_session = 1;
	 */
	CLI_CTX(&ctx)->node_filter = PRIV_FILTER(1);
	CLI_CTX(&ctx)->root = &menu_main;
	CLI_CTX(&ctx)->out_open = cli_out_open;
	RLSHELL_CTX(&ctx)->prompt = swcli_prompt;
	RLSHELL_CTX(&ctx)->exit = 0;
	RLSHELL_CTX(&ctx)->enable_ctrl_z = 0;
	ctx.sock_fd = -1;
	ctx.cdp = NULL;

	rlshell_main(RLSHELL_CTX(&ctx));

	return 0;
}

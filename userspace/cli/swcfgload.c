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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/limits.h>

#include "swcli.h"
extern struct menu_node config_main;

int load_main_config(struct swcli_context *ctx) {
	FILE *fp;
	char cmd[1024];	

	if ((fp = fopen(SW_CONFIG_FILE, "r")) == NULL) {
		return 1;
	}

	while (fgets(cmd, sizeof(cmd), fp)) {
		cmd[strlen(cmd) - 1] = '\0'; /* FIXME we don't crash, but it's ugly */
		if (cmd[0] == '!')
			continue;
		if (cmd[0] != ' ')
			CLI_CTX(ctx)->root = &config_main;
		rlshell_exec(RLSHELL_CTX(ctx), cmd);
	}

	fclose(fp);
	return 0;
}

int load_tag_config(struct swcli_context *ctx, int argc, char **argv, int silent) {
	/* argv[0] is if_name;
	 * argv[1] is tag_name;
	 * argv[2] is description, if argc > 2.
	 */

	FILE *fp;
	char cmd[1024];	
	char path[PATH_MAX];

	if (snprintf(cmd, sizeof(cmd), "interface netdev %s", argv[0]) > sizeof(cmd)) {
		fprintf(stderr, "Invalid interface: %s\n", argv[0]);
		return 1;
	}

	if (snprintf(path, sizeof(path), "%s/%s", SW_TAGS_FILE, argv[1]) > sizeof(path)) {
		fprintf(stderr, "Invalid tag: %s\n", argv[1]);
		return 1;
	}
	
	if ((fp = fopen(path, "r")) == NULL) {
		if (silent)
			return 0;
		fprintf(stderr, "Could not open %s\n", path);
		return 1;
	}

	if (rlshell_exec(RLSHELL_CTX(ctx), cmd)) {
		/* FIXME maybe we get more detailed information from
		 * command exit status (which now is not implemented at all :P)
		 */
		fprintf(stderr, "Could not add interface %s", argv[0]);
		fclose(fp);
		return 1;
	}

	do {
		if (argc <= 2)
			break;
		if (snprintf(cmd, sizeof(cmd), "description %s", argv[2]) < sizeof(cmd))
			break;
		rlshell_exec(RLSHELL_CTX(ctx), cmd);
	} while (0);
		

	while (fgets(cmd, sizeof(cmd), fp)) {
		cmd[strlen(cmd) - 1] = '\0'; /* FIXME we don't crash, but it's ugly */
		if (cmd[0] == '!')
			continue;
		if (cmd[0] != ' ')
			break;
		rlshell_exec(RLSHELL_CTX(ctx), cmd);
	}

	fclose(fp);
	return 0;
}

int main(int argc, char **argv) {
	int ret, opt, load_main = 0, silent = 0, out_help = 0;
	char *myself = argv[0];
	struct swcli_context ctx;

	while ((opt = getopt(argc, argv, "msh")) != -1) {
		switch (opt) {
		case 'm':
			load_main = 1;
			break;
		case 's':
			silent = 1;
			break;
		case 'h':
		default:
			out_help = 1;
		}
	}
	argc -= optind;
	argv += optind;

	do {
		if (out_help || load_main)
			break;
		if (argc >= 2 && argc <= 3)
			break;
		out_help = 1;
	} while(0);

	if (out_help) {
		fprintf(stderr, "Usage:\n"
				"  %s -m \n"
				"    Load main configuration from %s.\n"
				"  %s [-s] <if_name> <tag_name> [<description>]\n"
				"    Load configuration in %s/<tag_name> into interface\n"
				"    <if_name> and optionally assign description <description>\n"
				"    to the interface <if_name>.\n",
				myself, SW_CONFIG_FILE, myself, SW_TAGS_FILE);
		return 1;
	}

	CLI_CTX(&ctx)->node_filter = PRIV_FILTER(15);
	CLI_CTX(&ctx)->root = &config_main;
	CLI_CTX(&ctx)->out_open = cli_out_open;
	ctx.sock_fd = -1;
	ctx.cdp = NULL;

	if (switch_init() < 0) {
		perror("switch_init");
		return -1;
	}

	ctx.sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	if (ctx.sock_fd ==  -1) {
		perror("socket");
		return 1;
	}

	ret = load_main ? load_main_config(&ctx) :
		load_tag_config(&ctx, argc, argv, silent);

	close(ctx.sock_fd);

	return ret;
}

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

#include "climain.h"
#include "shared.h"
#include "config.h"
#include "cdpd.h"
#include "cdp_ipc.h"
#include "switch_socket.h"

extern int sock_fd;
extern sw_command_root_t *cmd_root;
extern int priv;
extern sw_execution_state_t exec_state;
extern sw_completion_state_t cmpl_state;

int cdp_enabled = 0;
char cdp_queue_name[32], our_queue_name[32];
mqd_t sq, rq;

FILE *out;

int swcfgload_exec(sw_execution_state_t *exc) {
	
	if (exc->num >= exc->size) {
		exc->func_args = realloc(exc->func_args,
				(exc->size + 1) *sizeof(char *));
		assert(exc->func_args);
		exc->size ++;
	}
	exc->func_args[exc->num++] = NULL;

	exc->func(out, exc->func_args);
	return 0;
	/* FIXME make exc->func (all handlers) return a status code;
	 * useful for example for loading interface configuration
	 * from /etc/lisa/tags to know if the "interface <if_name>"
	 * command failed (and abort the process)
	 */
}

int swcfgload_exec_cmd(char *cmd) {
	int ret;
	
	init_exec_state(&exec_state);
	ret = parse_command(strdup(cmd), lookup_token);
	
	if (ret <= 0) {
		fprintf(stderr, "Parse Error. Extra input at offset %d "
				"in command: '%s'\n", abs(ret), cmd);
		exit(1);
	}

	if (ret > 1 || (exec_state.runnable & NA)) {
		fprintf(stderr, "Ambiguous command: '%s'\n", cmd);
		exit(1);
	}

	if (!(exec_state.runnable & RUN)) {
		fprintf(stderr, "Incomplete command: '%s'\n", cmd);
		exit(1);
	}

	if (exec_state.func == NULL) {
		fprintf(stderr, "Warning: Unimplemented command: '%s'\n", cmd);
		return 1;
	}

	return swcfgload_exec(&exec_state);
}

int load_main_config(void) {
	FILE *fp;
	char cmd[1024];	

	if ((fp = fopen(config_file, "r")) == NULL) {
		return 1;
	}

	while (fgets(cmd, sizeof(cmd), fp)) {
		cmd[strlen(cmd) - 1] = '\0'; /* FIXME we don't crash, but it's ugly */
		if (cmd[0] == '!')
			continue;
#ifndef USE_EXIT_IN_CONF
		if (cmd[0] != ' ')
			cmd_root = &command_root_config;
#endif
		swcfgload_exec_cmd(cmd);
	}

	fclose(fp);
	return 0;
}

int load_tag_config(int argc, char **argv, int silent) {
	/* argv[0] is if_name;
	 * argv[1] is tag_name;
	 * argv[2] is description, if argc > 2.
	 */

	FILE *fp;
	char cmd[1024];	
	char path[PATH_MAX];

	if (snprintf(cmd, sizeof(cmd), "interface %s", argv[0]) > sizeof(cmd)) {
		fprintf(stderr, "Invalid interface: %s\n", argv[0]);
		return 1;
	}

	if (snprintf(path, sizeof(path), "%s/%s", config_tags_path, argv[1]) > sizeof(path)) {
		fprintf(stderr, "Invalid tag: %s\n", argv[1]);
		return 1;
	}
	
	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "Could not open %s\n", path);
		return 1;
	}

	if (swcfgload_exec_cmd(cmd)) {
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
		swcfgload_exec_cmd(cmd);
	} while (0);
		

	while (fgets(cmd, sizeof(cmd), fp)) {
		cmd[strlen(cmd) - 1] = '\0'; /* FIXME we don't crash, but it's ugly */
		if (cmd[0] == '!')
			continue;
#ifdef USE_EXIT_IN_CONF
		if (!strncmp(cmd, "exit", 4))
			break;
#else
		if (cmd[0] != ' ')
			break;
#endif
		swcfgload_exec_cmd(cmd);
	}

	fclose(fp);
	return 0;
}

int main(int argc, char **argv) {
	int status, ret, opt, load_main = 0, silent = 0, out_help = 0;
	char *myself = argv[0];

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
				myself, config_file, myself, config_tags_path);
		return 1;
	}

	priv = 15;
	
	status = cfg_init();
	assert(!status);

	out = fopen("/dev/null", "w");
	assert(out);
	
	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	if (sock_fd ==  -1) {
		perror("socket");
		return 1;
	}

	cdp_init_ipc(&cdp_s);

	cmd_root = &command_root_config;
	ret = load_main ? load_main_config() :
		load_tag_config(argc, argv, silent);

	fclose(out);
	cdp_destroy_ipc(&cdp_s);
	close(sock_fd);

	return ret;
}

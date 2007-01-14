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

#include "climain.h"
#include "config.h"
#include "cdpd.h"
#include "cdp_ipc.h"
#include "switch_socket.h"

extern int sock_fd;
extern sw_command_root_t *cmd_root;
extern int priv;
extern sw_execution_state_t exec_state;
extern sw_completion_state_t cmpl_state;
int cdp_ipc_qid;
int cdp_enabled;
pid_t my_pid;
FILE *out;

void swcfgload_exec(sw_execution_state_t *exc) {
	
	if (exc->num >= exc->size) {
		exc->func_args = realloc(exc->func_args,
				(exc->size + 1) *sizeof(char *));
		assert(exc->func_args);
		exc->size ++;
	}
	exc->func_args[exc->num++] = NULL;

	exc->func(out, exc->func_args);
}

void swcfgload_exec_cmd(char *cmd) {
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
		return;
	}

	swcfgload_exec(&exec_state);
}

int main(int argc, char *argv[]) {
	FILE *fp;
	char config_name[] = "/flash/config.text";
	char cmd[1024];	
	int status;
	
	priv = 15;
	cmd_root = &command_root_config;
	
	status = cfg_init();
	assert(!status);

	out = fopen("/dev/null", "w");
	assert(out);
	
	sock_fd = socket(PF_SWITCH, SOCK_RAW, 0);
	if (sock_fd ==  -1) {
		perror("socket");
		return 1;
	}
	
	my_pid = getpid();
	dbg("my pid: %d\n", my_pid);
	if ((cdp_ipc_qid = msgget(CDP_IPC_QUEUE_KEY, 0666)) == -1) {
		perror("CDP ipc queue doesn't exist. Is cdpd running?");
		fclose(out);
		close(sock_fd);
		return 1;
	}
	dbg("cdp_ipc_qid: %d\n", cdp_ipc_qid);
	cdp_enabled = 1;
	
	if ((fp = fopen(config_name, "r")) == NULL) {
		fclose(out);
		close(sock_fd);
		return 1;
	}	
	while (fgets(cmd, sizeof(cmd), fp)) {
		cmd[strlen(cmd)-1] = '\0';
		if (cmd[0] == '!')
			continue;
#ifndef USE_EXIT_IN_CONF
		if (cmd[0] != ' ')
			cmd_root = &command_root_config;
#endif
		swcfgload_exec_cmd(cmd);
	}
	fclose(out);
	fclose(fp);
	close(sock_fd);
	return 0;
}

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
#include <linux/reboot.h>
int reboot(int flag);
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGINT,	SIG_IGN);

	if (kill(1, SIGTSTP) < 0) {
		fprintf(stderr, "shutdown: can't idle init.\r\n");
		exit(1);
	}

	kill(-1, SIGTERM);
	sleep(1);
	kill(-1, SIGKILL);
	sleep(1);

	sync();
	reboot(LINUX_REBOOT_CMD_RESTART);
	return 0;
}

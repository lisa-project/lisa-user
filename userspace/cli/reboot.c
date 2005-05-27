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

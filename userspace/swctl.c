#include <stdio.h>
#include <string.h>

#include <linux/sockios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <assert.h>

int main(int argc, char **argv) {
	int sock;
	int status;

	if(!strcmp(argv[1], "add")) {
		sock = socket(PF_PACKET, SOCK_RAW, 0);
		if(sock == -1)
			perror("socket");
		status = ioctl(sock, SIOCSWADDIF, argv[2]);
		if(status)
			perror("add failed");
		return 0;
	}

	if(!strcmp(argv[1], "del")) {
		sock = socket(PF_PACKET, SOCK_RAW, 0);
		if(sock == -1)
			perror("socket");
		status = ioctl(sock, SIOCSWDELIF, argv[2]);
		if(status)
			perror("del failed");
		return 0;
	}

	return 0;
}

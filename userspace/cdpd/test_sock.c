#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/net_switch.h>
#include <linux/socket.h>

#include "switch_socket.h"

int main(int argc, char **argv) {
	int fd, err;
	struct sockaddr_sw addr;

	fd = socket(PF_SWITCH, SOCK_RAW, 0);
	if(fd == -1) {
		perror("socket");
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.ssw_family = AF_SWITCH;
	strncpy(addr.ssw_if_name, "eth1", sizeof(addr.ssw_if_name) - 1);
	addr.ssw_proto = ETH_P_CDP;
	err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if(err) {
		perror("bind");
		return 1;
	}

	return 0;
}

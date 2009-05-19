#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/net_switch.h>
#include <linux/socket.h>

#include "swsock.h"
#include "rstp_bpdu.h"
#include "rstp_utils.h"

//TODO include define from kernel header
#define ETH_P_RSTP 0x0022

int main(int argc, char **argv) {
	int fd, err, len, i;
	struct sockaddr_sw addr;
	char buf[255];

	/* Test create() */
	fd = socket(PF_SWITCH, SOCK_RAW, 0);
	if(fd == -1) {
		perror("socket");
		return 1;
	}

	/* Test bind() */
	memset(&addr, 0, sizeof(addr));
	addr.ssw_family = AF_SWITCH;
	strncpy(addr.ssw_if_name, "eth1", sizeof(addr.ssw_if_name) - 1);
	addr.ssw_proto = ETH_P_RSTP;
	err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if(err) {
		close(fd);
		perror("bind");
		return 1;
	}

	/* Test recvmsg() */
	if ((len = recv(fd, buf, 255, 0)) < 0) {
		perror("recv");
	}
	else {
		printf("Received packet of length %d.\n", len);
		printf("Data dump:");
		for (i = 0; i<len; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf("%02x ", buf[i] & 0xff);
		}
		printf("\n");
	}

	dissect_frame((struct stp_bpdu_t *)buf);
/*
	 Test sendmsg()
	memset(buf, 0, sizeof(buf));
	for (i=0; i<sizeof(buf); i++)
		buf[i] = i;
	if (send(fd, buf, sizeof(buf), 0) < 0)
		perror("send");

	 Test release() */
	close(fd);

	return 0;
}

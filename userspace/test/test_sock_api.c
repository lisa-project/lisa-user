#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/net_switch.h>
#include <linux/socket.h>
#include <pcap.h>
#include <netinet/in.h>
#include "../include/socket_api.h"

#include "swsock.h"

int main(int argc, char **argv) {
	int fd, err;
	pcap_t *pcap;
	u_char *packet;

	if (argc != 2) {
		printf("Usage: ./test_sock_api if_name\n");
		return 1;
	}

	/* Test create() */
	fd = socket(PF_PACKET, SOCK_RAW, IPPROTO_IP);
	if(fd == -1) {
		perror("socket");
		return 1;
	}

	/* Test filter */
	err = register_filter(&pcap, NO_FILTER, argv[1]);
	if (err < 0) {
		close(fd);
		perror("register_filter");
		return 1;
	}

	if (pcap == NULL) {
		printf("Value not available ");
		return -1;
	}

	/* Test recvmsg() */
	packet = recv_packet(pcap);
	if (NULL == packet) {
		close(fd);
		perror("recv packet");
		return 1;
	}

	printf("Data dump len: %d\n",strlen((char *)packet));

	/* Test release() */
	close(fd);

	return 0;
}

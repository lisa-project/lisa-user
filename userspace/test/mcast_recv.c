#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>

#include <stdio.h>

int main(int argc, char **argv) {
	int sock, stat;
	char buf[4096];
	struct ip_mreq mreq;
	int reuse = 1;
	struct sockaddr_in local_addr;
	const char *mcast_addr = "224.5.0.80";
	const char *if_addr = "10.12.0.1";

	if (argc >= 2)
		if_addr = argv[1];

	if (argc >= 3)
		mcast_addr = argv[2];

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assert(sock != -1);

	stat = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	assert(stat != -1);

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(2000);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	stat = bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));

	inet_aton(mcast_addr, &mreq.imr_multiaddr);
	inet_aton(if_addr, &mreq.imr_interface);
	setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

	for (;;) {
		ssize_t s;
		s = recv(sock, buf, sizeof(buf), 0);
		if (s == -1) {
			perror("sendto: ");
			continue;
		}
		buf[s] = '\0'; //FIXME
		printf("Received: %s\n", buf);
	}
}


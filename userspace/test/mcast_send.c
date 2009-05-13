#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>

#include <stdio.h>

int main(int argc, char **argv) {
	int sock, status;
	struct sockaddr_in dst;
	char buf[] = "Hello, world!";
	const char *mcast_addr = "224.5.0.80";
	u_char ttl = 64;

	if (argc >= 2)
		mcast_addr = argv[1];

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assert(sock != -1);

	dst.sin_family = AF_INET;
	dst.sin_port = htons(2000);
	status = inet_aton(mcast_addr, &dst.sin_addr);
	assert(status);
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	for (;;) {
		ssize_t s;
		s = sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&dst, sizeof(dst));
		if (s == -1)
			perror("sendto: ");
		usleep(250000);
	}

	return 0;
}

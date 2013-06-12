#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int sock, status;
	char buf[2048];
	ssize_t s;
	struct sockaddr_in from;
	const struct sockaddr *__from = (const struct sockaddr *)&from;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assert(sock != -1);

	from.sin_family = AF_INET;
	from.sin_port = htons(0xbeef);
	from.sin_addr.s_addr = INADDR_ANY;
	status = bind(sock, __from, sizeof(from));
	assert(!status);

	do {
		s = recv(sock, buf, sizeof(buf), 0);
		if (s == -1) {
			perror("recv");
			continue;
		}
		printf("Received %zd byte(s). buf[0] is '%c'\n", s, buf[0]);
	} while(s);

	return 0;
}

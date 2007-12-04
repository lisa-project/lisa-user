#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int sock, status;
	char buf[1] = "A";
	ssize_t s;
	struct sockaddr_in to, from;
	const struct sockaddr *__to = (const struct sockaddr *)&to;
	const struct sockaddr *__from = (const struct sockaddr *)&from;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	assert(sock != -1);

	from.sin_family = AF_INET;
	from.sin_port = htons(0xdead);
	from.sin_addr.s_addr = INADDR_ANY;
	status = bind(sock, __from, sizeof(from));
	assert(!status);

	to.sin_family = AF_INET;
	to.sin_port = htons(0xbeef);
	status = inet_aton("192.168.222.1", &to.sin_addr);
	assert(status);

	s = sendto(sock, buf, sizeof(buf), MSG_CONFIRM, __to, sizeof(to));
	assert(s == sizeof(buf));
	printf("Successfully sent %d byte(s)\n", s);

	return 0;
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <unistd.h>
#include <syslog.h>

#include "list.h"
#include "debug.h"
#include "swclid.h"

struct telnet_conn {
	pid_t pid;
	struct sockaddr_in remote_addr;
	struct list_head lh;
};

int telnet_sockfd;
int yes = 1;
LIST_HEAD(telnet_conns);

void daemonize() {
}

void telnet_listen() {
	int status;
	struct sockaddr_in bind_addr;

	telnet_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(telnet_sockfd != -1);
	
	status = setsockopt(telnet_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int));
	assert(status != -1);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(23);
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	status = bind(telnet_sockfd, (struct sockaddr *)&bind_addr,
			sizeof(struct sockaddr));
	assert(status != -1);

	status = listen(telnet_sockfd, 10);
	assert(status != -1);
}

struct telnet_conn *create_telnet_conn() {
	struct telnet_conn *c;
	struct rlimit rlim;
	struct sockaddr_in remote_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int fd;
	int i, status;

	c = malloc(sizeof(struct telnet_conn));
	assert(c);

	switch((c->pid = fork())) {
	case 0:
		/* we are the child */
		break;
	case -1:
		syslog(LOG_ERR, "child stage 1 failed");
		return NULL;
	default:
		/* we are the parent */
		return c;
	}

	/* close all open file descriptors, so that the accepted connection
	   automatically becomes fd 0
	 */
	status = getrlimit(RLIMIT_NOFILE, &rlim);
	if(status || rlim.rlim_max < 0) {
		syslog(LOG_ERR, "child stage 2 failed");
		exit(1);
	}

	for(i = 0; i < rlim.rlim_max; i++) {
		if(i == telnet_sockfd)
			continue;
		close(i);
	}

	fd = accept(telnet_sockfd, (struct sockaddr *)&remote_addr, &addrlen);
	close(telnet_sockfd);
	if(fd) {
		syslog(LOG_ERR, "child stage 3 failed");
		exit(1);
	}
	dup(fd);
	dup(fd);

	do {
		char * argv[] = {TELNETD_PATH, "-h", "-L", LOGIN_PATH, NULL};
		char * envp[] = {NULL};

		syslog(LOG_INFO, "New telnet session from %s",
				inet_ntoa(remote_addr.sin_addr));
		status = execve(TELNETD_PATH, argv, envp);
	} while(0);

	syslog(LOG_ERR, "child stage 4 failed");
	exit(1);
}

void destroy_telnet_conn(struct telnet_conn *c) {
	list_del(&c->lh);
	free(c);
}

void sgh_child(int sig) {
	pid_t pid;
	int status;
	struct telnet_conn *c, *tmp;

	pid = waitpid(-1, &status, WNOHANG);
	switch(pid) {
	case 0:
		/* No child exited, but we handle SIGCHLD... wtf ?!! */
		return;
	case -1:
		//perror("waitpid"); /* FIXME */
		return;
	}
	list_for_each_entry_safe(c, tmp, &telnet_conns, lh) {
		if(c->pid != pid)
			continue;
		destroy_telnet_conn(c);
		return;
	}
}

#define MAXFD(x) (maxfd = (x) > maxfd ? (x) : maxfd)
void main_loop() {
	int status;
	int maxfd;
	fd_set readfds, writefds;
	struct telnet_conn *c;

	do {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(telnet_sockfd, &readfds);
		maxfd = telnet_sockfd;

		status = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
		/* FIXME status == 0 ?? */
		if(status == -1) {
			//perror("select");
			//fflush(stderr);
		}

		if(FD_ISSET(telnet_sockfd, &readfds)) {
			c = create_telnet_conn();
			if(c != NULL)
				list_add_tail(&c->lh, &telnet_conns);
		}
	} while(1);
}
#undef MAXFD

int main(int argc, char **argv) {
	openlog("swclid", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "started");
	daemonize();
	telnet_listen();
	signal(SIGCHLD, sgh_child);
	main_loop();
	return 0;
}

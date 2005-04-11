#include <assert.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/telnet.h>

#include "list.h"
#include "debug.h"
#include "climain.h"
#include "ascii_cname.h"

struct fdqueue {
	unsigned char buf[4096];
	int head, tail;

	struct list_head lh;
};

#define WF_EXITED		0x01
#define WF_READY		0x02
#define WF_SOCKCLOSED	0x04

struct worker {
	pid_t pid;
	int flags;
	struct sockaddr_in remote_addr;

	int sockfd;
	int ptyfd;

	struct list_head sock_q;
	struct list_head pty_q;

	struct list_head lh;
};

int srv_sockfd;
int yes = 1;
LIST_HEAD(workers);
struct termios pty_termios;

void __daemonize() {
}

void __listen() {
	int status;
	struct sockaddr_in bind_addr;

	srv_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(srv_sockfd != -1);
	
	status = setsockopt(srv_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int));
	assert(status != -1);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(8023);
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	status = bind(srv_sockfd, (struct sockaddr *)&bind_addr,
			sizeof(struct sockaddr));
	assert(status != -1);

	status = listen(srv_sockfd, 10);
	assert(status != -1);
}

int __handle_read(int fd, struct list_head *lh) {
	int status;
	struct fdqueue *q;

	q = malloc(sizeof(struct fdqueue));
	assert(q);
	status = read(fd, q->buf, sizeof(q->buf));
	dbg("Read %d bytes from fd %d\n", status, fd);
	if(status <= 0) {
		free(q);
		return status;
	}
	q->head = 0;
	q->tail = status;
	dump_mem(q->buf, status);
	list_add_tail(&q->lh, lh);
	return status;
}

void __handle_write(int fd, struct list_head *lh) {
	int status;
	struct fdqueue *q;

	if(list_empty(lh)) {
		assert(0);
		return;
	}
	q = list_entry(lh->next, struct fdqueue, lh);
	status = write(fd, q->buf + q->head, q->tail - q->head);
	dbg("Wrote %d bytes to fd %d\n", status, fd);
	/* FIXME status == -1 */
	if((q->head += status) >= q->tail) {
		list_del(&q->lh);
		free(q);
	}
}

#define SEND(fd,par...) do {\
	unsigned char cmd[] = {par};\
	write(fd, cmd, sizeof(cmd));\
} while(0)

void init_telnet(int fd) {
	SEND(fd, IAC, WILL, TELOPT_ECHO);
	SEND(fd, IAC, WILL, TELOPT_SGA);
	SEND(fd, IAC, DONT, TELOPT_LINEMODE);
	SEND(fd, IAC, DO, TELOPT_NAWS);
}

struct worker *create_worker() {
	struct worker *w;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	w = malloc(sizeof(struct worker));
	assert(w);

	w->flags = 0;
	INIT_LIST_HEAD(&w->sock_q);
	INIT_LIST_HEAD(&w->pty_q);

	w->sockfd = accept(srv_sockfd, (struct sockaddr *)&w->remote_addr, &addrlen);
	assert(w->sockfd != -1);

	switch((w->pid = forkpty(&w->ptyfd, NULL, NULL, NULL))) {
	case 0:
		/* we are the child */
		init_telnet(1);
		climain();
		exit(0);
	case -1:
		assert(0);
		break;
	}
	dbg("Created new worker(%d): sockfd=%d ptyfd=%d\n", w->pid,
			w->sockfd, w->ptyfd);

	return w;
}

void __destroy_q(struct list_head *lh) {
	struct fdqueue *q, *tmp;
	list_for_each_entry_safe(q, tmp, lh, lh) {
		list_del(&q->lh);
		free(q);
	}
}

void destroy_worker(struct worker *w) {
	list_del(&w->lh);
	__destroy_q(&w->sock_q);
	__destroy_q(&w->pty_q);
	close(w->sockfd);
	close(w->ptyfd);
	dbg("Free worker(%d)\n", w->pid);
	free(w);
}

void sgh_child(int sig) {
	pid_t pid;
	int status;
	struct worker *w, *tmp;

	pid = waitpid(-1, &status, WNOHANG);
	switch(pid) {
	case 0:
		/* No child exited, but we handle SIGCHLD... wtf ?!! */
		return;
	case -1:
		perror("waitpid"); /* FIXME */
		return;
	}
	list_for_each_entry_safe(w, tmp, &workers, lh) {
		if(w->pid == pid)
			break;
	}
	assert(w->pid == pid);
	if(w->flags & WF_SOCKCLOSED) {
		/* the socket was closed; the loop function will skip entries
		   for this worker, so we can clean it here */
		destroy_worker(w);
		return;
	}
	/* the child exited by itself; we mark it for deletion and let
	   the loop function clean it after all data from the pty is sent
	   over the socket
	 */
	dbg("Child with pid %d slated for deletion\n", pid);
	w->flags |= WF_EXITED;
}

#define MAXFD(x) (maxfd = (x) > maxfd ? (x) : maxfd)
void __loop() {
	int status;
	int maxfd;
	fd_set readfds, writefds;
	struct worker *w, *tmp;

	do {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(srv_sockfd, &readfds);
		maxfd = srv_sockfd;
		list_for_each_entry_safe(w, tmp, &workers, lh) {
			if(w->flags & WF_SOCKCLOSED)
				continue;
			dbg("Processing fds for pid %d\n", w->pid);
			if(!list_empty(&w->sock_q)) {
				dbg("Add sockfd=%d to writefds\n", w->sockfd);
				FD_SET(w->sockfd, &writefds);
			} else {
				if(w->flags & WF_EXITED) {
					if(w->flags & WF_READY) {
						destroy_worker(w);
						continue;
					} else
						w->flags |= WF_READY;
				}
			}
			dbg("Add sockfd=%d to readfds\n", w->sockfd);
			FD_SET(w->sockfd, &readfds);
			MAXFD(w->sockfd);
			if(!list_empty(&w->pty_q)) {
				if(w->flags & WF_EXITED)
					__destroy_q(&w->pty_q);
				else {
					dbg("Add ptyfd=%d to writefds\n", w->ptyfd);
					FD_SET(w->ptyfd, &writefds);
				}
			}
			dbg("Add ptyfd=%d to readfds\n", w->ptyfd);
			FD_SET(w->ptyfd, &readfds);
			MAXFD(w->ptyfd);
		}
		status = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
		/* FIXME status == 0 ?? */
		if(status == -1) {
			perror("select");
			fflush(stderr);
		}
		if(FD_ISSET(srv_sockfd, &readfds)) {
			/* new connection */
			w = create_worker();
			list_add_tail(&w->lh, &workers);
		}
		list_for_each_entry(w, &workers, lh) {
			if(FD_ISSET(w->sockfd, &readfds) &&
					__handle_read(w->sockfd, &w->pty_q) <= 0) {
				w->flags |= WF_SOCKCLOSED;
				kill(w->pid, SIGKILL);
				continue;
			}
			if(FD_ISSET(w->sockfd, &writefds))
				__handle_write(w->sockfd, &w->sock_q);
			if(FD_ISSET(w->ptyfd, &readfds))
				__handle_read(w->ptyfd, &w->sock_q);
			if(FD_ISSET(w->ptyfd, &writefds))
				__handle_write(w->ptyfd, &w->pty_q);
		}
	} while(1);
}
#undef MAXFD

void __init_pty_termios(struct termios *t) {
	t->c_iflag = 0;
	t->c_oflag = 0;
	t->c_cflag = 0;
	t->c_lflag = 0;
	cfmakeraw(t);
	
	t->c_cc[VINTR]       = C_ETX;
	t->c_cc[VQUIT]       = C_FS;
	t->c_cc[VERASE]      = C_DEL;
	t->c_cc[VKILL]       = C_NAK;
	t->c_cc[VEOF]        = C_EOT;
	t->c_cc[VTIME]       = 1; /* FIXME */
	t->c_cc[VMIN]        = 1; /* FIXME */
	t->c_cc[VSWTC]       = C_NUL;
	t->c_cc[VSTART]      = C_DC1;
	t->c_cc[VSTOP]       = C_DC3;
	t->c_cc[VSUSP]       = C_SUB;
	t->c_cc[VEOL]        = C_NUL;
	t->c_cc[VREPRINT]    = C_DC2;
	t->c_cc[VDISCARD]    = C_SI;
	t->c_cc[VWERASE]     = C_ETB;
	t->c_cc[VLNEXT]      = C_SYN;
	t->c_cc[VEOL2]       = C_NUL;
}

int main(int argc, char **argv) {
	__daemonize();
	__listen();
	signal(SIGCHLD, sgh_child);
	__init_pty_termios(&pty_termios);
	__loop();
	return 0;
}

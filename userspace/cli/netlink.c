/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#include "netlink.h"


/* Scope: RT_SCOPE_HOST if lcl->data begins with 127 */
int default_scope(inet_prefix *lcl) {
	if (lcl->family == AF_INET) {
		if (lcl->bytelen >= 1 && *(__u8*)&lcl->data == 127)
			return RT_SCOPE_HOST;
	}
	return 0;
}

/* Parse long value in some base from string pointed by arg */
int get_integer(int *val, const char *arg, int base) {
	long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtol(arg, &ptr, base);
	if (!ptr || ptr == arg || *ptr || res > INT_MAX || res < INT_MIN)
		return -1;
	*val = res;
	return 0;
}

/* 
 	Build inet prefix, parsing the string name.
	Returns -1 on parse error or 0 on success.
*/
static int get_addr_1(inet_prefix *addr, const char *name, int family) {
	const char *cp;
	unsigned char *ap = (unsigned char*)addr->data;
	int i;

	memset(addr, 0, sizeof(*addr));

	addr->family = AF_INET;
	addr->bytelen = 4;
	addr->bitlen = -1;
	for (cp=name, i=0; *cp; cp++) {
		if (*cp <= '9' && *cp >= '0') {
			ap[i] = 10*ap[i] + (*cp-'0');
			continue;
		}
		if (*cp == '.' && ++i <= 3)
			continue;
		return -1;
	}
	return 0;
}

/* 
  Parse inet addr into inet_prefix.
  	arg = a.b.c.d[/mask]
 */
static int get_prefix_1(inet_prefix *dst, const char *arg, int family) {
	int err = 0;
	unsigned plen;
	char *slash;

	memset(dst, 0, sizeof(*dst));

	slash = strchr(arg, '/');
	if (slash)
		*slash = 0;
	err = get_addr_1(dst, arg, family);
	if (err == 0) {
		dst->bitlen = 32;
		if (slash) {
			if (get_integer(&plen, slash+1, 0) || plen > dst->bitlen) {
				err = -1;
				goto done;
			}
			dst->bitlen = plen;
		}
	}
done:
	if (slash)
		*slash = '/';
	return err;
}

/* Calls get_prefix_1 to parse inet addr from arg to an inet_prefix */
int get_prefix(inet_prefix *dst, const char *arg, int family) {
	int err;
	
	if ((err = get_prefix_1(dst, arg, family))) {
		fprintf(stderr, "Error: an inet prefix is expected rather than \"%s\".\n", arg);
	}
	return err;
}

int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max)
			tb[rta->rta_type] = rta;
		rta = RTA_NEXT(rta,len);
	}
	if (len)
		fprintf(stderr, "!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
	return 0;
}

int addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data, 
	      int alen) {
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		fprintf(stderr, "addattr_l ERROR: message exceeded bound of %d\n",maxlen);
		return -1;
	}
	rta = (struct rtattr*)(((char*)n) + NLMSG_ALIGN(n->nlmsg_len));
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;
	return 0;
}

/* Open netlink socket */
int rtnl_open(struct rtnl_handle *rth) {
	int addr_len;
	int sndbuf = 32768;
	int rcvbuf = 32768;

	memset(rth, 0, sizeof(rth));

	rth->fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (rth->fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_SNDBUF,&sndbuf,sizeof(sndbuf)) < 0) {
		perror("SO_SNDBUF");
		return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf)) < 0) {
		perror("SO_RCVBUF");
		return -1;
	}

	memset(&rth->local, 0, sizeof(rth->local));
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_groups = 0;

	if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}
	addr_len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(rth->local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		return -1;
	}
	if (rth->local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", rth->local.nl_family);
		return -1;
	}
	rth->seq = time(NULL);
	return 0;
}

int rtnl_wilddump_request(struct rtnl_handle *rth, int family, int type) {
	struct {
		struct nlmsghdr nlh;
		struct rtgenmsg g;
	} req;
	struct sockaddr_nl nladdr;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = type;
	req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
	req.nlh.nlmsg_seq = rth->dump = ++rth->seq;
	req.g.rtgen_family = family;

	return sendto(rth->fd, (void*)&req, sizeof(req), 0, (struct sockaddr*)&nladdr, sizeof(nladdr));
}

int rtnl_dump_filter(struct rtnl_handle *rth, rtnl_filter_t filter, void *arg) {
	char	buf[16384];
	struct sockaddr_nl nladdr;
	struct iovec iov = { buf, sizeof(buf) };

	while (1) {
		int status;
		struct nlmsghdr *h;

		struct msghdr msg = {
			(void*)&nladdr, sizeof(nladdr),
			&iov,	1,
			NULL,	0,
			0
		};

		status = recvmsg(rth->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			perror("OVERRUN");
			continue;
		}
		if (status == 0) {
			fprintf(stderr, "EOF on netlink\n");
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			fprintf(stderr, "sender address length == %d\n", msg.msg_namelen);
			return -1;
		}

		h = (struct nlmsghdr*)buf;
		while (NLMSG_OK(h, status)) {
			int err;

			if (nladdr.nl_pid != 0 ||
					h->nlmsg_pid != rth->local.nl_pid ||
					h->nlmsg_seq != rth->dump)
				goto skip_it;

			if (h->nlmsg_type == NLMSG_DONE)
				return 0;
			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
				if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					fprintf(stderr, "ERROR truncated\n");
				} else {
					errno = -err->error;
					perror("RTNETLINK answers");
				}
				return -1;
			}
			err = filter(&nladdr, h, arg);
			if (err < 0)
				return err;

skip_it:
			h = NLMSG_NEXT(h, status);
		}

		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "Message truncated\n");
			continue;
		}
		if (status) {
			fprintf(stderr, "!!!Remnant of size %d\n", status);
			return -1;
		}
	}

	return 0;
}

int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n) {
	int status;
	unsigned seq;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov = { (void*)n, n->nlmsg_len };
	char   buf[16384];
	struct msghdr msg = {
		(void*)&nladdr, sizeof(nladdr),
		&iov,	1,
		NULL,	0,
		0
	};

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	n->nlmsg_seq = seq = ++rtnl->seq;
	n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(rtnl->fd, &msg, 0);

	if (status < 0) {
		perror("Cannot talk to rtnetlink");
		return -1;
	}

	memset(buf,0,sizeof(buf));

	iov.iov_base = buf;

	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(rtnl->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
				continue;
			perror("OVERRUN");
			continue;
		}
		if (status == 0) {
			fprintf(stderr, "EOF on netlink\n");
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			fprintf(stderr, "sender address length == %d\n", msg.msg_namelen);
			return -1;
		}
		for (h = (struct nlmsghdr*)buf; status >= sizeof(*h); ) {
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);

			if (l<0 || len>status) {
				if (msg.msg_flags & MSG_TRUNC) {
					fprintf(stderr, "Truncated message\n");
					return -1;
				}
				fprintf(stderr, "!!!malformed message: len=%d\n", len);
				return -1;
			}

			if (nladdr.nl_pid != 0 ||
					h->nlmsg_pid != rtnl->local.nl_pid ||
					h->nlmsg_seq != seq)
				continue;

			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
				if (l < sizeof(struct nlmsgerr)) {
					fprintf(stderr, "ERROR truncated\n");
				} else {
					errno = -err->error;
					if (errno == 0)
						return 0;
					perror("RTNETLINK answers");
				}
				return -1;
			}
			
			fprintf(stderr, "Unexpected reply!!!\n");

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
		}
		
		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "Message truncated\n");
			continue;
		}
		if (status) {
			fprintf(stderr, "!!!Remnant of size %d\n", status);
			return -1;
		}
	}

	return 0;
}

/* Close netlink socket */
void rtnl_close(struct rtnl_handle *rth) {
	close(rth->fd);
}

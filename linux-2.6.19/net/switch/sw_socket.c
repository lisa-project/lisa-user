/*
 *    This file is part of Linux Multilayer Switch.
 *
 *    Linux Multilayer Switch is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Linux Multilayer Switch is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Linux Multilayer Switch; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/protocol.h>
#include <net/sock.h>

#include "sw_private.h"
#include "sw_debug.h"

/* This implements the PF_SWITCH protocol family that is used by user space
 * daemons (such as cdpd) to send and receive protocol-specific raw packets.
 *
 * We prefer this over PF_PACKET and libpcap/libnet because we have to
 * detect and filter these packets anyway in the switching engine. By using
 * our custom socket implementation, we can "send" these packets to user
 * space instead of just dropping them.
 *
 * This implementation is strongly inspired from PF_PACKET implementation,
 * in net/packet/af_packet.c.
 */

/* Custom structure that wraps the kernel "sock" struct. We use it to be
 * able to add implementation-specific data to the socket structure.
 */
struct switch_sock {
	/* struct sock has to be the first member of packet_sock to keep
	 * consistency with the rest of the kernel */
	struct sock			sk;

	/* implementation-specific fields follow */
	int					proto;
};

static inline struct switch_sock *sw_sk(struct sock *sk) {
	return (struct switch_sock *)sk;
}

/* Almost copy-paste from af_packet.c */
static void sw_sock_destruct(struct sock *sk) {
	BUG_TRAP(!atomic_read(&sk->sk_rmem_alloc));
	BUG_TRAP(!atomic_read(&sk->sk_wmem_alloc));

	if (!sock_flag(sk, SOCK_DEAD)) {
		printk("Attempt to release alive switch socket: %p\n", sk);
		return;
	}

	//atomic_dec(&packet_socks_nr); FIXME
}

static struct proto switch_proto = {
	.name = "SWITCH",
	.owner = THIS_MODULE,
	.obj_size = sizeof(struct switch_sock)
};

static const struct proto_ops sw_sock_ops;

static int sw_sock_create(struct socket *sock, int protocol) {
	struct sock *sk;
	struct switch_sock *sws;

	if(!capable(CAP_NET_RAW))
		return -EPERM;
	if(sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;

	sock->state = SS_UNCONNECTED;

	sk = sk_alloc(PF_SWITCH, GFP_KERNEL, &switch_proto, 1);
	if(sk == NULL)
		return -ENOBUFS;

	sock->ops = &sw_sock_ops;
	sock_init_data(sock, sk);

	sws = sw_sk(sk);
	sk->sk_family = PF_SWITCH;
	sws->proto = protocol;

	sk->sk_destruct = sw_sock_destruct;

	return 0;
}

static int sw_sock_release(struct socket *sock) {
	//FIXME must implement
	return 0;
}

static int sw_sock_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len) {
	//FIXME must implement
	return 0;
}

static int sw_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg) {
	//FIXME must implement, if we plan to migrate "deviceless ioctls" here :)
	return 0;
}

static int sw_sock_sendmsg(struct kiocb *iocb, struct socket *sock,
		struct msghdr *msg, size_t len) {
	//FIXME must implement
	return 0;
}

static int sw_sock_recvmsg(struct kiocb *iocb, struct socket *sock,
		struct msghdr *msg, size_t len, int flags) {
	//FIXME must implement
	return 0;
}

static const struct proto_ops sw_sock_ops = {
	.family =		PF_SWITCH,
	.owner =		THIS_MODULE,
	.release = 		sw_sock_release,
	.bind =			sw_sock_bind,
	.connect =		sock_no_connect,
	.socketpair =	sock_no_socketpair,
	.accept =		sock_no_accept,
	.getname =		sock_no_getname,
	.poll =			datagram_poll,
	.ioctl =		sw_sock_ioctl,
	.listen =		sock_no_listen,
	.shutdown =		sock_no_shutdown,
	.setsockopt =	sock_no_setsockopt,
	.getsockopt =	sock_no_getsockopt,
	.sendmsg =		sw_sock_sendmsg,
	.recvmsg =		sw_sock_recvmsg,
	.mmap =			sock_no_mmap,
	.sendpage =		sock_no_sendpage,
};

static struct net_proto_family switch_family_ops = {
	.family =		PF_SWITCH,
	.create =		sw_sock_create,
	.owner =		THIS_MODULE,
};

void sw_sock_exit(void) {
	sock_unregister(PF_SWITCH);
	proto_unregister(&switch_proto);
	dbg("Sucessfully unregistered PF_SWITCH protocol family\n");
}

int sw_sock_init(void) {
	int err = proto_register(&switch_proto, 0);

	if(err)
		goto out;
	
	sock_register(&switch_family_ops);
	dbg("Sucessfully registered PF_SWITCH protocol family\n");
out:
	return err;
}

MODULE_ALIAS_NETPROTO(PF_SWITCH);

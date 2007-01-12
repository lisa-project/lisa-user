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

DEFINE_MUTEX(sw_ioctl_mutex);

/* Custom structure that wraps the kernel "sock" struct. We use it to be
 * able to add implementation-specific data to the socket structure.
 */
struct switch_sock {
	/* struct sock has to be the first member of switch_sock to keep
	 * consistency with the rest of the kernel */
	struct sock			sk;

	/* implementation-specific fields follow */
	int					proto;			/* socket protocol number */
	struct list_head	port_chain;		/* link to port list of sockets */
};

static inline struct switch_sock *sw_sk(struct sock *sk) {
	return (struct switch_sock *)sk;
}

/* Almost copy-paste from af_packet.c */
static void sw_sock_destruct(struct sock *sk) {
	dbg("sw_sock_destruct, sk=%p\n", sk);
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

	dbg("sw_sock_create, proro=%d\n", protocol);
	if(!capable(CAP_NET_RAW))
		return -EPERM;
	if(sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;
	if(protocol)
		return -ESOCKTNOSUPPORT;

	sock->state = SS_UNCONNECTED;

	sk = sk_alloc(PF_SWITCH, GFP_KERNEL, &switch_proto, 1);
	if(sk == NULL)
		return -ENOBUFS;

	sock->ops = &sw_sock_ops;
	sock_init_data(sock, sk);

	sws = sw_sk(sk);
	sk->sk_family = PF_SWITCH;
	sws->proto = 0;

	sk->sk_destruct = sw_sock_destruct;

	return 0;
}

static int bind_switch_port(struct switch_sock *sws, struct net_switch_port *port, int proto) {
	struct list_head *lh;

	switch(proto) {
	case ETH_P_CDP:
		lh = &port->sock_cdp;
		break;
	default:
		return -EINVAL;
	}
	//FIXME prepare socket for reception
	sws->proto = proto;

	/* now the socket is ready and we can publish it to the port */
	list_add_tail_rcu(&sws->port_chain, lh);
	return 0;
}

static void unbind_switch_port(struct switch_sock *sws) {
	if(!sws->proto)
		return;
	list_del_rcu(&sws->port_chain);
	synchronize_rcu();
	sws->proto = 0;
	//FIXME flush queues now?
}

static int sw_sock_release(struct socket *sock) {
	struct switch_sock *sws = sw_sk(sock->sk);
	
	dbg("sw_sock_release, sock=%p\n", sock);
	unbind_switch_port(sws); //FIXME sw_ioctl_mutex is not locked
	//FIXME must implement
	return 0;
}

static int sw_sock_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len) {
	struct net_device *dev = NULL;
	struct switch_sock *sws = sw_sk(sock->sk);
	struct sockaddr_sw *sw_addr = (struct sockaddr_sw *)uaddr;
	int err;

	dbg("sw_sock_bind, sk=%p\n", sws);

	if(addr_len < sizeof(struct sockaddr_sw))
		return -EINVAL;
	if(sw_addr->ssw_family != AF_SWITCH)
		return -EINVAL;
	//FIXME copy_from_user before using uaddr ?
	dev = dev_get_by_name(sw_addr->ssw_if_name);
	if(dev == NULL)
		return -ENODEV;
	
	/* prevent devices from being added or removed from the switch */
	mutex_lock(&sw_ioctl_mutex);

	if(dev->sw_port == NULL) {
		err = -ENODEV; /* FIXME: better suited value */
	} else {
		unbind_switch_port(sws);
		err = bind_switch_port(sws, dev->sw_port, sw_addr->ssw_proto);
	}

	mutex_unlock(&sw_ioctl_mutex);
	dev_put(dev); /* dev_get_by_name() calls dev_hold() */
	return err;
}

/* First implementation of Linux Multilayer Switch used a hook in
 * sock_ioctl(), which is the socket-specific implementation
 * of the ioctl() generic file operation. It is referenced in
 * the socket_file_ops structure at the beginnig of socket.c.
 *
 * In sock_ioctl() we added a new branch to the switch statement:
 * our own hook to call our particular ioctl() handler. But the
 * default branch of the switch statement calls sock->ops->ioctl(),
 * which points to sw_sock_ioctl().
 *
 * Now that we have our own protocol implementation, we prefer
 * using our own ioctl() handler than hijacking the main socket
 * ioctl() handler.
 */
static int sw_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg) {
	int err;
	void __user *argp = (void __user *)arg;

	mutex_lock(&sw_ioctl_mutex);
	err = sw_deviceless_ioctl(cmd, argp);
	mutex_unlock(&sw_ioctl_mutex);
	return err;
}

static int sw_sock_sendmsg(struct kiocb *iocb, struct socket *sock,
		struct msghdr *msg, size_t len) {
	dbg("sw_sock_sendmsg, sk=%p\n", sock->sk);
	//FIXME must implement
	return 0;
}

static int sw_sock_recvmsg(struct kiocb *iocb, struct socket *sock,
		struct msghdr *msg, size_t len, int flags) {
	dbg("sw_sock_recvmsg, sk=%p\n", sock->sk);
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

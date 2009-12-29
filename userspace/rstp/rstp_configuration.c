#include "rstp.h"

/**
 * Configuration management via a POSIX message queue.
 */
extern struct list_head registered_interfaces;

char rstp_queue_name[32];

static void 
rstp_ipc_send_response(struct rstp_request *m, char *r) {
	char qname[32];
	mqd_t sq;

	memset(qname, 0, sizeof(qname));
	snprintf(qname, sizeof(qname), RSTP_QUEUE_NAME, m->pid);
	qname[sizeof(qname)-1] = '\0';

	if ((sq = mq_open(qname, O_WRONLY)) == -1) {
		sys_dbg("[ipc listener] Unable to open queue \
			'%s' for sending response.\n", qname);
		return;
	}

	if (mq_send(sq, (const char *)r, RSTP_MAX_RESPONSE_SIZE, 0) < 0) {
		sys_dbg("[ipc listener] Failed to send message \
			on queue'%s'.\n", qname);
		return;
	}

	mq_close(sq);
}

void*
rstp_ipc_listen(void *arg)
{
	char r[RSTP_MAX_RESPONSE_SIZE];
	struct rstp_request *m;
	struct mq_attr attr;
	mqd_t rq;
	char *msg;

	memset(rstp_queue_name, 0, sizeof(rstp_queue_name));
	snprintf(rstp_queue_name, sizeof(rstp_queue_name), RSTP_QUEUE_NAME, 0);
	rstp_queue_name[sizeof(rstp_queue_name) - 1] = '\0';

	/* We open the queue with O_EXCL because we need to assure we're the
 	 * only rstp instance running. */
	if ((rq = mq_open(rstp_queue_name, O_CREAT|O_EXCL|O_RDONLY, 0666, NULL)) == -1) {
		sys_dbg("[ipc listener] Failed to create queue \
			'%s'\n", rstp_queue_name);
		exit(EXIT_FAILURE);
	}

	if (mq_getattr(rq, &attr) < 0) {
		sys_dbg("[ipc listener] Failed to get queue attributes\n");
		exit(EXIT_FAILURE);
	}

	if (!(msg = malloc(attr.mq_msgsize + 1))) {
		sys_dbg("[ipc listener] Failed to allocate receive buffer\n");
		exit(EXIT_FAILURE);
	}

	sys_dbg("[ipc listener] listening for commands\n");

	/* loop listening for incoming requests */
	for (;;) {
		memset(msg, 0, attr.mq_msgsize + 1);

		/* receive a message from the message queue */
		if (mq_receive(rq, msg, attr.mq_msgsize, NULL) < 0) {
			sys_dbg("[ipc listener] Message receive failed\n");
			exit(EXIT_FAILURE);
		}

		m = (struct rstp_request *)msg;

		/* interpret & compose response */
		sys_dbg("[ipc listener] received message of type: %d\n", m->type);
		sys_dbg("[ipc listener] sender pid is %d\n", m->pid);

		memset(r, 0, RSTP_MAX_RESPONSE_SIZE);

		switch (m->type) {
		case RSTP_IF_ENABLE:
			sys_dbg("[ipc listener] %s: rstp if_enable, if_index=%d\n",
				__func__, m->if_index);
			rstpd_register_interface(m->if_index);
			break;
		case RSTP_IF_DISABLE:
			sys_dbg("[ipc listener] %s: rstp if_disable, if_index=%d\n",
				__func__, m->if_index);
			rstpd_unregister_interface(m->if_index);
			break;
		default:
			sys_dbg("[ipc listener] %s: unknown message type %d\n",
				__func__, m->type);
		}

		/* send the response */
		rstp_ipc_send_response(m, r);
	}

	free(msg);
	pthread_exit(NULL);
}


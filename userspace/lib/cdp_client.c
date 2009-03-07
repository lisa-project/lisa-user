#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "cdp_client.h"

/* Initiates a cdp client session */
struct cdp_session *cdp_session_start(void) {
	struct cdp_session *s;
	struct mq_attr attr;

	/* Be paranoid about user input */
	s = (struct cdp_session *)malloc(sizeof(struct cdp_session));
	assert(s);
	memset(s, 0, sizeof(struct cdp_session));

	/* Try to open the send message queue on which we send requests to cdpd */
	memset(s->sq_name, 0, sizeof(s->sq_name));
	snprintf(s->sq_name, sizeof(s->sq_name), CDP_QUEUE_NAME, 0);
	s->sq_name[sizeof(s->sq_name)-1] = '\0';

	if ((s->sq = mq_open(s->sq_name, O_WRONLY)) < 0)
		goto out_err;

	/* Open the message queue for receiving responses from cdpd */
	memset(s->rq_name, 0, sizeof(s->rq_name));
	snprintf(s->rq_name, sizeof(s->rq_name), CDP_QUEUE_NAME, getpid());
	s->rq_name[sizeof(s->rq_name)-1] = '\0';

	if ((s->rq = mq_open(s->rq_name, O_CREAT|O_RDONLY, 0666, NULL)) < 0)
		goto out_err;

	/* Get the max message size attribute */
	if (mq_getattr(s->rq, &attr) < 0)
		goto out_err;

	s->max_msg_len = attr.mq_msgsize;

	/* Alloc the receive message buffer */
	if (!(s->response = malloc(s->max_msg_len)))
		goto out_err;

	return s;

out_err:
	cdp_session_end(s);
	return NULL;
}

/* Ends a cdp client session */
void cdp_session_end(struct cdp_session *s) {
	assert(s);

	if (s->response)
		free(s->response);
	mq_close(s->rq);
	mq_unlink(s->rq_name);

	return;
}

/* timed receive for a message from the client queue */
int cdp_session_recv(struct cdp_session *s) {
	struct timespec ts;
	time_t ns;

	assert(s);

	memset(&ts, 0, sizeof(ts));
	time(&ns);
	ts.tv_sec = ns + CDP_CLIENT_TIMEOUT;

	if (mq_timedreceive(s->rq, s->response,  s->max_msg_len, NULL, &ts) < 0) {
		fprintf(stderr, "No message received from cdpd in %d seconds.\n"
			"Disabling cdp in this session.\n", CDP_CLIENT_TIMEOUT);
		return -EAGAIN;
	}

	return 0;
}

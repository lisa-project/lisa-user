#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"
#include "if_generic.h"
#include "rstp_client.h"

struct rstp_session *
rstp_session_start(void)
{
	struct rstp_session *s;
	struct mq_attr attr;

	if (!(s = (struct rstp_session *)malloc(sizeof(struct rstp_session))))
		return NULL;

	memset(s, 0, sizeof(struct rstp_session));
	s->sq = s->rq = -1;	

	/* Try to open the send message queue on which we send requests to rstpd */
	memset(s->sq_name, 0, sizeof(s->sq_name));
	snprintf(s->sq_name, sizeof(s->sq_name), RSTP_QUEUE_NAME, 0);
	s->sq_name[sizeof(s->sq_name) - 1] = '\0';

	if ((s->sq = mq_open(s->sq_name, O_WRONLY)) < 0) {
		perror("Could not open send queue\n");
		goto out_err;
	}

	/* Open the message queue for receiving responses from rstpd */
	memset(s->rq_name, 0, sizeof(s->rq_name));
	snprintf(s->rq_name, sizeof(s->rq_name), RSTP_QUEUE_NAME, getpid());
	s->rq_name[sizeof(s->rq_name) - 1] = '\0';

	if ((s->rq = mq_open(s->rq_name, O_CREAT | O_RDONLY, 0666, NULL)) < 0) {
		printf("Could not open recv queue\n");
		goto out_err;
	}

	if (mq_getattr(s->rq, &attr) < 0) {
		printf("Could not get attributes\n");
		goto out_err;
	}

	s->max_msg_len = attr.mq_msgsize;

	if (!(s->response = malloc(s->max_msg_len))) {
		printf("Could not allocate mem\n");
		goto out_err;
	}

	return s;

out_err:
	rstp_session_end(s);
	return NULL;
}

void
rstp_session_end(struct rstp_session *s)
{
	assert(s);

	if (s->response)
		free(s->response);
	if (s->sq != -1) {
		mq_close(s->sq);
	}
	if (s->rq != -1) {
		mq_close(s->rq);
		mq_unlink(s->rq_name);
	}
	free(s);
}


int
rstp_session_recv(struct rstp_session *s)
{
	struct timespec ts;
	time_t ns;
	int err;

	assert(s);

	memset(&ts, 0, sizeof(ts));
	time(&ns);
	ts.tv_sec = ns + RSTP_CLIENT_TIMEOUT;

	if ((err = mq_timedreceive(s->rq, s->response, s->max_msg_len, NULL, &ts)) < 0) {
		fprintf(stderr, "No message received from rstpd in %d seconds.\n"
			"Disabling rstp in this session.\n", RSTP_CLIENT_TIMEOUT);
	}

	return err;
}

int
rstp_set_interface(struct rstp_session *session, int if_index, int enabled)
{
	struct rstp_request m;
	int err;

	assert(session);
	memset(&m, 0, sizeof(m));
	m.type = enabled ? RSTP_IF_ENABLE : RSTP_IF_DISABLE;
	m.pid = getpid();
	m.if_index = if_index;

	if ((err = rstp_session_send(session, m)) < 0) {
		perror("rstp_session_send");
		return err;
	}

	return 10; //rstp_session_recv(session);
}

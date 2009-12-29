#ifndef _RSTP_UTILS_H__
#define _RSTP_UTILS_H__

void dissect_frame(struct stp_bpdu_t * stpframe);

/* RSTP stuff here */
struct port_timers {
	unsigned int edgeDelayWhile;
	unsigned int fdWhile;
	unsigned int helloWhen;
	unsigned int mdelayWhile;
	unsigned int rbWhile;
	unsigned int rcvdInfoWhile;
	unsigned int rrWhile;
	unsigned int tcWhile;
};

#endif



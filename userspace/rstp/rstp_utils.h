#ifndef _RSTP_UTILS_H__
#define _RSTP_UTILS_H__

#include "vector.h"

void dissect_frame(struct bpdu_t * stpframe);

void print_vector(struct priority_vector4 *);

#endif


#include <string.h>
#include "vector.h"

int vec_compare(struct priority_vector v1, struct priority_vector v2) {
	return memcmp(&v1, &v2, sizeof(struct priority_vector));
}

int vec_compare4(struct priority_vector4 v1, struct priority_vector4 v2) {
	return memcmp(&v1, &v2, sizeof(struct priority_vector4));
}

int tim_compare(struct rstp_times t1, struct rstp_times t2) {
	return memcmp(&t1, &t2, sizeof(struct rstp_times));
}


#ifndef _VECTOR_H
#define _VECTOR_H

#include <string.h>

struct bridge_id {
	unsigned char bridge_priority[2];
	unsigned char bridge_address[6];
} __attribute__((packed));

struct priority_vector {
	struct bridge_id root_bridge_id;
	unsigned char root_path_cost[4];
	struct bridge_id designated_bridge_id;
	unsigned char designated_port_id[2];
	unsigned char bridge_port_id[2];
} __attribute__((packed));

struct priority_vector4 {
	struct bridge_id root_bridge_id;
	unsigned char root_path_cost[4];
	struct bridge_id designated_bridge_id;
	unsigned char designated_port_id[2];
} __attribute__((packed));

struct rstp_times {
	unsigned short MessageAge;
	unsigned short MaxAge;
	unsigned short HelloTime;
	unsigned short ForwardDelay;
};

int vec_compare(struct priority_vector, struct priority_vector);

int vec_compare4(struct priority_vector4, struct priority_vector4);

int tim_compare(struct rstp_times, struct rstp_times);

#endif

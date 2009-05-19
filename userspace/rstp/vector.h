#ifndef _PRIO_VECTOR_H__
#define _PRIO_VECTOR_H__

struct prio_vec_t {
	unsigned char *root_id;
	unsigned long root_path_cost;
	unsigned char *designated_id;
	unsigned char *designated_port;
	unsigned char *bridge_port;
};

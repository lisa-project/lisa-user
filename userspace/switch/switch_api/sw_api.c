
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "mm.h"
#include "list.h"
#include "sw_api.h"
#include "if_generic.h"

/**
 * Look-up the switch which corresponds to the received index and return
 * its value
 *
 * @param sw_index the index of the switch
 * @return the associated entry from struct backend_entries
 */
static struct backend_entries* get_switch_entry(int sw_index)
{
	int counter = 0;
	struct backend_entries *iter_sw;

	list_for_each_entry(iter_sw, &head_sw_ops, lh) {
		if (sw_index == DEFAULT_SW) {
			if (strcmp(iter_sw->type, LINUX_BACKEND) == 0) {
				return iter_sw;
			}
		} else {
			if (counter == sw_index) {
				return iter_sw;
			}
		}
		++counter;
	}

	return NULL;
}

/**
 * Verify if an interface is part of a switch
 */
static int interface_in_switch(struct backend_entries *entry, char *if_name)
{
	struct switch_interface *sw_if_iter;

	list_for_each_entry(sw_if_iter, &entry->if_names_lh, lh) {
		if (strcmp(sw_if_iter->if_name, if_name) == 0)
			return 0;
	}

	return 1;
}

int if_add(int sw_index, char *if_name, int mode)
{
	int if_index, sock_fd;
	struct backend_entries *entry;

	/* get switch back-end which corresponds to the switch index */
	if (NULL == (entry = get_switch_entry(sw_index)))
		return 1;

	/* verify if the switch contains a certain interface */
	if (interface_in_switch(entry, if_name) < 0)
		return 1;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return 1;

	if_index = if_get_index(if_name, sock_fd);
	if (if_index <= 0)
		return 1;

	close(sock_fd);

	return entry->sw_ops->if_add(entry->sw_ops, if_index, mode);
}


int if_remove(int sw_index, char *if_name)
{
	struct backend_entries *entry;
	int sock_fd, if_index;

	/* find the switch that contains the interface */
	if (NULL == (entry = get_switch_entry(sw_index)))
		return 1;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return 1;

	if_index = if_get_index(if_name, sock_fd);
	if (if_index <= 0)
		return 1;

	close(sock_fd);

	/* delete the interface from the identified switch */
	return entry->sw_ops->if_remove(entry->sw_ops, if_index);
}


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
		if (strcmp(iter_sw->locality, SW_LOCAL) == 0) {
			if (sw_index == DEFAULT_SW) {
				if (strcmp(iter_sw->type, LINUX_BACKEND) == 0) {
					return iter_sw;
				}
			} else {
				if (counter == sw_index) {
					return iter_sw;
				}
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

/**
 * For a given interface name and switch index it returns the entry
 * associated with the switch and the index of the interface
 * 
 * The index of the interface is returned to the caller function as a
 * side effect.(*if_index parameter)
 */
static struct backend_entries *get_if_params(int sw_index, char *if_name, int *if_index)
{
	int sock_fd;
	struct backend_entries *entry;

	/* get switch back-end which corresponds to the switch index */
	if (NULL == (entry = get_switch_entry(sw_index)))
		return NULL;

	/* verify if the switch contains a certain interface */
	if (interface_in_switch(entry, if_name) < 0)
		return NULL;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return NULL;

	*if_index = if_get_index(if_name, sock_fd);
	if (*if_index <= 0)
		return NULL;

	close(sock_fd);

	return entry;
}


int if_add(int sw_index, char *if_name, int mode)
{
	struct backend_entries *entry;
	int if_index;

	entry = get_if_params(sw_index, if_name, &if_index);
	if (NULL == entry)
		return 1;

	return entry->sw_ops->if_add(entry->sw_ops, if_index, mode);
}


int if_remove(int sw_index, char *if_name)
{
	struct backend_entries *entry;
	int if_index;

	entry = get_if_params(sw_index, if_name, &if_index);
	if (NULL == entry)
		return 1;

	/* delete the interface from the identified switch */
	return entry->sw_ops->if_remove(entry->sw_ops, if_index);
}

int if_set_mode(int sw_index, int if_index, int mode, int flag)
{
	struct backend_entries *entry;
	char *if_name;
	int sock_fd;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return 1;

	if_name = if_get_name(if_index, sock_fd, NULL);
	if (if_name == NULL)
		return 1;

	close(sock_fd);

	entry = get_switch_entry(sw_index);
	if (NULL == entry)
		return 1;

	if (0 != interface_in_switch(entry, if_name))
		return 1;

	/* delete the interface from the identified switch */
	return entry->sw_ops->if_set_mode(entry->sw_ops, if_index, mode, flag);
}


int if_set_port_vlan(int sw_index, int if_index, int vlan)
{
	struct backend_entries *entry;
	char *if_name;
	int sock_fd;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return 1;

	if_name = if_get_name(if_index, sock_fd, NULL);
	if (if_name == NULL)
		return 1;

	close(sock_fd);

	entry = get_switch_entry(sw_index);
	if (NULL == entry)
		return 1;

	if (0 != interface_in_switch(entry, if_name))
		return 1;

	return entry->sw_ops->if_set_port_vlan(entry->sw_ops, if_index, vlan);
}


int if_get_type_api(int sw_index, char *if_name, int *type, int *vlan)
{
	struct backend_entries *entry;
	int if_index;

	entry = get_if_params(sw_index, if_name, &if_index);
	if (NULL == entry)
		return 1;

	return entry->sw_ops->if_get_type(entry->sw_ops, if_index, type, vlan);
}

int if_enable(int sw_index, char *if_name)
{

	struct backend_entries *entry;
	int if_index;

	entry = get_if_params(sw_index, if_name, &if_index);
	if (NULL == entry)
		return 1;

	return entry->sw_ops->if_enable(entry->sw_ops, if_index);
}

int if_disable(int sw_index, char *if_name)
{
	struct backend_entries *entry;
	int if_index;

	entry = get_if_params(sw_index, if_name, &if_index);
	if (NULL == entry)
		return 1;

	return entry->sw_ops->if_disable(entry->sw_ops, if_index);
}

int vlan_add(int vlan)
{
	struct backend_entries *iter_sw;
	int status, counter = 0;

	list_for_each_entry(iter_sw, &head_sw_ops, lh) {
		if (strcmp(iter_sw->locality, SW_LOCAL) == 0) {
			status = iter_sw->sw_ops->vlan_add(iter_sw->sw_ops, vlan);
			if (status) {
				goto del_vlan;
			}
			++counter;
		}
	}
	return 0;

del_vlan:
	list_for_each_entry(iter_sw, &head_sw_ops, lh) {
		if (strcmp(iter_sw->locality, SW_LOCAL)) {
			if (counter == 0)
				break;

			status = iter_sw->sw_ops->vlan_del(iter_sw->sw_ops, vlan);
			if (status != 0)
				return -1;

			--counter;
		}
	}

	return -1;
}

int vlan_del(int vlan)
{
	struct backend_entries *iter_sw;
	int status;

	list_for_each_entry(iter_sw, &head_sw_ops, lh) {
		if (strcmp(iter_sw->locality, SW_LOCAL) == 0) {
			status = iter_sw->sw_ops->vlan_del(iter_sw->sw_ops, vlan);
			if (status != 0)
				return -1;
		}
	}

	return 0;
}

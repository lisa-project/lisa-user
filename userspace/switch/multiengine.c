/*
  *    this file is part of lisa multi-engine implementation
  *
  *    lisa command line interface is free software; you can redistribute it 
  *    and/or modify it under the terms of the gnu general public license 
  *    as published by the free software foundation; either version 2 of the 
  *    license, or (at your option) any later version.
  *
  *    lisa command line interface is distributed in the hope that it will be 
  *    useful, but without any warranty; without even the implied warranty of
  *    merchantability or fitness for a particular purpose.  see the
  *    gnu general public license for more details.
  *
  *    you should have received a copy of the gnu general public license
  *    along with lisa command line interface; if not, write to the free 
  *    software foundation, inc., 59 temple place, suite 330, boston, 
  *    ma  02111-1307  usa
  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#include "multiengine.h"

#define get_switch_item(entry_ptr, entry_name, json_ptr) do {\
	char *tmp = NULL; \
	tmp = cJSON_GetObjectItem((json_ptr), entry_name)->valuestring; \
	if (NULL != tmp) {\
		memcpy((entry_ptr)->entry_name, tmp, strlen(tmp));\
		((entry_ptr)->entry_name)[strlen(tmp)] = '\0';\
		free(tmp); \
	}\
} while(0)

static char type[] 	= "type";
static char locality[] 	= "locality";
static char ip[]	= "ip";
static char port[]	= "port";
static char if_name[]	= "if_name";

struct list_head head_sw_ops;

void multiengine_init(void)
{
	INIT_LIST_HEAD(&head_sw_ops);
}

int open_so_local(char *so_name, struct sw_ops_entries *entry)
{
	void *handle;
	char *error;
	void (*register_switch_so)(struct switch_operations*);

	/* open switch libraries */
	handle = dlopen(so_name, RTLD_LAZY);
	if (NULL == handle) {
		printf("Error opening library [%s] : %s\n", so_name, dlerror());
		return -1;
	}

	dlerror();
	register_switch_so = (void*)dlsym(handle, "register_switch");
	if ((error = dlerror()) != NULL)  {
		printf("Error dlsym(): %s\n", error);
		exit(1);
	}

	entry->sw_ops = malloc(sizeof(struct switch_operations));
	if (NULL == entry->sw_ops) {
		printf("Couldn't allocate space\n");
		return -1;
	}

	(*register_switch_so)(entry->sw_ops);

	dlclose(handle);
	return 0;
}

/* obtain the name of the shared object associated with the switch and
 * open the object for registration */
int get_shared_object(cJSON *item, struct sw_ops_entries *entry)
{
	char *so_name;
	int ret;

	so_name = cJSON_GetObjectItem(item, "shared_object")->valuestring;
	if (NULL == so_name)
		return -1;

	/* set the current entry in the list of sw_ops_entries to be
	 * populated with the sw_ops structure */
	ret =  open_so_local(so_name, entry);

	return ret;
}

/* obtain the names of the interfaces or the ports associated with a switch */
int get_interface_names(cJSON *item, struct sw_ops_entries *entry)
{
	int size, idx;
	cJSON *root, *child;
	struct switch_interface *sw_if;

	INIT_LIST_HEAD(&entry->if_names_lh);

	/* get object containing all interfaces */
	root = cJSON_GetObjectItem(item, "interfaces");
	if (NULL == root)
		return -1;

	size = cJSON_GetArraySize(root);

	child = root->child;

	/* the switch contains interfaces */
	for (idx = 0; child && idx < size; ++idx, child = child->next) {
		sw_if = malloc(sizeof(struct switch_interface));
		if (NULL == sw_if)
			return -1;

		/* get the name of the interface */
		get_switch_item(sw_if, if_name, child);

		list_add_tail(&sw_if->lh, &entry->if_names_lh);
	}

	return 0;
}

/* parse the connfiguration file and populate the list associated with
 * the switches*/
int parse_config_file(char *data)
{
	cJSON *root, *sw_items, *child;
	struct sw_ops_entries *entry;
	int item_idx, size;

	root = cJSON_Parse(data);
	if (NULL == root) {
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}

	/* get array of switch items */
	sw_items = cJSON_GetObjectItem(root, JSON_SWITCH_NODE);
	size = cJSON_GetArraySize(sw_items);

	child = sw_items->child;

	for (item_idx = 0; child && item_idx < size; ++item_idx, child = child->next) {
		/* alloc a new entry */
		entry = malloc(sizeof(struct sw_ops_entries));
		if (NULL == entry) {
			printf("Error allocating new entry\n");
			return -1;
		}

		entry->sw_index = item_idx;

		/* save switch name */
		get_switch_item(entry, type, child);

		/* get switch locality */
		get_switch_item(entry, locality, child);

		/* for local switches we don't have to save IP and port
		 * number */
		if (strcmp(entry->locality, SW_LOCAL) != 0) {
			get_switch_item(entry, ip, child);
			get_switch_item(entry, port, child);
		}

		/* obtain the name of the shared object open it */
		if (strcmp(entry->locality, SW_LOCAL) == 0) {
			if (-1 == get_shared_object(child, entry)) {
				printf("Bad locality\n");
				free(entry);
				return -1;
			}
		}

		/* save interface names */
		if (-1 == get_interface_names(child, entry)) {
			printf("Bad names\n");
			free(entry);
			return -1;
		}

		list_add_tail(&entry->lh, &head_sw_ops);
	}

	cJSON_Delete(root);
	free(data);

	return 0;
}

/* obtain content of configuration file */
char* read_config_file(void)
{
	int fd, read_size, read_total;
	off_t offset;
	char *data;

	/* open configuration file */
	fd = open(CONFIG_FILENAME, O_RDONLY);
	if (-1 == fd)
		return NULL;

	/* go to the end of file */
	offset = lseek(fd, 0, SEEK_END);
	if (-1 == offset)
		return NULL;

	if (-1 == lseek(fd, 0, SEEK_SET))
		return NULL;

	data = (char*)malloc(offset + 1);

	read_size = 0;
	read_total = 0;
	while((read_size = read(fd, data + read_total, offset)) != 0) {
		offset -= read_size;
		read_total += read_size;
	}

	close(fd);

	return data;
}


void print_lists(void)
{
	struct sw_ops_entries *iter_sw;
	struct switch_interface *iter_names;

	printf("\n---- LIST of SWICHES ----");
	list_for_each_entry(iter_sw, &head_sw_ops, lh)  {
		printf("\n[SW_IDX] %d\n", iter_sw->sw_index);
		printf("\tType: %s\n",iter_sw->type);
		printf("\tLocality %s\n", iter_sw->locality);
		if (strcmp(iter_sw->locality, SW_LOCAL) != 0) {
			printf("\tIP: %s\n", iter_sw->ip);
			printf("\tPort: %s\n", iter_sw->port);
		} else {
			printf("\tSw_ops ptr: %p\n", iter_sw->sw_ops);
		}

		printf("\tInterface Names:\n\t\t|");
		list_for_each_entry(iter_names, &iter_sw->if_names_lh, lh) {
			printf("%10s|", iter_names->if_name);
		}

	}
	printf("\n");
}

int main()
{
	char *data;
	multiengine_init();
	if(NULL == (data = read_config_file()))
		return -1;

	if(-1 == parse_config_file(data)) {
		printf("Failed parsing config file\n");
		return -1;
	}


	print_lists();
	return 0;
}


/*
 *    This file is part of LiSA Switch
 *
 *    LiSA Switch is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2 of the
 *    License, or (at your option) any later version.
 *
 *    LiSA Switch is distributed in the hope that it will be
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Switch; if not, write to the Free
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 */

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

static struct switch_operations* get_switch_operations(struct list_head head_sw_ops, int sw_index)
{
	int counter = 0;
	struct sw_ops_entries *iter_sw;

	printf("Switch index is %d\n", sw_index);
	
	list_for_each_entry(iter_sw, &head_sw_ops, lh) {
		printf("Enter iterator\n");
		if (sw_index == DEFAULT_SW) {
			if (strcmp(iter_sw->type, LINUX_BACKEND) == 0) {
				printf("found linux back-end\n");
				return iter_sw->sw_ops;
			}
		} else {
			if (counter == sw_index) {
				return iter_sw->sw_ops;
			}
		}
		++counter;
	}

	return NULL;
}

int if_add(struct list_head head_sw_ops, int sw_index, char *if_name, int mode)
{
	int if_index, sock_fd;
	struct switch_operations *sw_ops;

	printf("Enter interface add multiengine\n");
	print_lists(head_sw_ops);
	/* get switch operations which correspond to the switch
	 * implementation */
	if (NULL == (sw_ops = get_switch_operations(head_sw_ops, sw_index)))
		return 1;

	/* cast the name of the interface into an interface index */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
		return 1;

	if_index = if_get_index(if_name, sock_fd);
	if (if_index <= 0)
		return 1;

	printf("Interface index is %d\n", if_index);
	close(sock_fd);

	return sw_ops->if_add(sw_ops, if_index, mode);
}

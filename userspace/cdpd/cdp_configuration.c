/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#include "cdpd.h"

/**
 * Configuration management via a POSIX message queue.
 */
extern struct list_head registered_interfaces;
extern struct cdp_traffic_stats cdp_stats;

char cdp_queue_name[32];

static void cdp_ipc_add_neighbor(struct cdp_neighbor *n, struct cdp_interface *entry, char *ptr) {
	struct cdp_neighbor_info *info;

	/* copy the neighbor info into the buffer pointed by ptr */
	memcpy(ptr, &n->info, sizeof(struct cdp_neighbor_info));
	info = (struct cdp_neighbor_info *)ptr;
	info->if_index = entry->if_index;
	info->ttl = n->hnode->tstamp - time(NULL);
}

/**
 * Fetch cdp registered interfaces into the cdp response
 */
static int cdp_ipc_get_interfaces(char *cdpr, int if_index) {
	struct cdp_interface *entry, *tmp;
	int count = 0;
	char *ptr = cdpr + sizeof(int);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		sem_wait(&entry->n_sem);
		if (if_index && if_index != entry->if_index) {
			sem_post(&entry->n_sem);
			continue;
		}
		count++;
		*((int *)ptr) = entry->if_index;
		ptr += sizeof(int);
		sem_post(&entry->n_sem);
		if (if_index)
			break;
	}

	return count;
}

/**
 * Fetch all known neighbors into the cdp response.
 *
 * A value of 0 for if_index means all interfaces and a value of
 * NULL disables filtering by device_id.
 */
static int cdp_ipc_get_neighbors(char *cdpr, int if_index, char *device_id) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0;
	char *ptr = (char *) cdpr + sizeof(int);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		/* filter out unwanted interfaces */
		if (if_index && if_index != entry->if_index)
			continue;

		sem_wait(&entry->n_sem);

		list_for_each_entry_safe(n, t, &(entry->neighbors), lh) {
			/* filter out unwanted device ids */
			if (device_id && strncmp(n->info.device_id, device_id, strlen(device_id)))
				continue;

			cdp_ipc_add_neighbor(n, entry, ptr);
			ptr += sizeof(struct cdp_neighbor_info);
			count++;

			/* not enough memory for another neighbor in cdpr->data  */
			if (CDP_MAX_RESPONSE_SIZE-sizeof(int) < (count+1) * sizeof(struct cdp_neighbor_info)) {
				sem_post(&entry->n_sem);
				return count;
			}
		}

		sem_post(&entry->n_sem);
	}
	return count;
}

static void cdp_ipc_send_response(struct cdp_request *m, char *r) {
	char qname[32];
	mqd_t sq;

	memset(qname, 0, sizeof(qname));
	snprintf(qname, sizeof(qname), CDP_QUEUE_NAME, m->pid);
	qname[sizeof(qname)-1] = '\0';

	if ((sq = mq_open(qname, O_WRONLY)) == -1) {
		sys_dbg("Unable to open queue '%s' for sending response.\n", qname);
		return;
	}

	if (mq_send(sq, (const char *)r, CDP_MAX_RESPONSE_SIZE, 0) < 0) {
		sys_dbg("Failed to send message on queue'%s'.\n", qname);
		return;
	}

	mq_close(sq);
}

void *cdp_ipc_listen(void *arg) {
	struct cdp_request *m;
	char r[CDP_MAX_RESPONSE_SIZE];
	struct mq_attr attr;
	char *msg;
	mqd_t rq;

	memset(cdp_queue_name, 0, sizeof(cdp_queue_name));
	snprintf(cdp_queue_name, sizeof(cdp_queue_name), CDP_QUEUE_NAME, 0);
	cdp_queue_name[sizeof(cdp_queue_name)-1] = '\0';

	/* We open the queue with O_EXCL because we need to assure we're the
	 * only cdpd instance running. */
	if ((rq = mq_open(cdp_queue_name, O_CREAT|O_EXCL|O_RDONLY, 0666, NULL)) == -1) {
		sys_dbg("Failed to create queue '%s'\n", cdp_queue_name);
		exit(1);
	}

	/* set the message size attribute */
	if (mq_getattr(rq, &attr) < 0) {
		sys_dbg("Failed to get queue attributes\n");
		exit(1);
	}

	if (!(msg = malloc(attr.mq_msgsize+1))) {
		sys_dbg("Failed to allocate receive buffer\n");
		exit(1);
	}

	sys_dbg("cdp ipc listen\n");
	/* loop listening for incoming requests */
	for (;;) {
		memset(msg, 0, attr.mq_msgsize+1);

		/* receive a message from the message queue */
		if (mq_receive(rq, msg, attr.mq_msgsize, NULL) < 0) {
			sys_dbg("Message receive failed\n");
			exit(1);
		}

		m = (struct cdp_request *)msg;

		/* interpret & compose response */
		sys_dbg("[ipc listener]: received message of type: %d\n", m->type);
		sys_dbg("[ipc listener]: sender pid is: %d\n", m->pid);

		memset(r, 0, CDP_MAX_RESPONSE_SIZE);

		switch (m->type) {
		case CDP_SHOW_STATS:
			sys_dbg("%s: cdp show stats\n", __func__);
			memcpy(r, &cdp_stats, sizeof(cdp_stats));
			break;
		case CDP_SHOW_INTF:
			sys_dbg("%s: cdp show intf, if_index=%d\n", __func__, m->if_index);
			*((int *) r) = cdp_ipc_get_interfaces(r, m->if_index);
			break;
		case CDP_SHOW_NEIGHBORS:
			sys_dbg("%s: cdp show neighbors, if_index=%d, device_id=%s\n",
					__func__, m->if_index, strlen(m->device_id)? m->device_id : NULL);
			*((int *)r) = cdp_ipc_get_neighbors(r, m->if_index,
					strlen(m->device_id)? m->device_id :  NULL);
			break;
		case CDP_IF_ENABLE:
			sys_dbg("%s: cdp if enable, if_index=%d\n", __func__, m->if_index);
			cdpd_register_interface(m->if_index);
			break;
		case CDP_IF_DISABLE:
			sys_dbg("%s: cdp if disable, if_index=%d\n", __func__, m->if_index);
			cdpd_unregister_interface(m->if_index);
			break;
		case CDP_IF_STATUS:
			sys_dbg("%s: cdp if status, if_index=%d\n", __func__, m->if_index);
			*((int *) r) = cdpd_get_interface_status(m->if_index);
			break;
		default:
			sys_dbg("%s: unknown message type %d\n", __func__, m->type);
		}

		/* send the response */
		cdp_ipc_send_response(m, r);
	}

	free(msg);
	pthread_exit(NULL);
}

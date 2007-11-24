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
#include "cdp_ipc.h"
#include "debug.h"

/**
 * Configuration management via a POSIX message queue.
 */
extern struct cdp_configuration ccfg;
extern struct list_head registered_interfaces;
extern struct cdp_traffic_stats cdp_stats;

extern void register_cdp_interface(char *);
extern void unregister_cdp_interface(char *);
extern int get_cdp_status(char *);

char cdp_queue_name[32];

static void cdp_ipc_add_neighbor(struct cdp_neighbor *n, struct cdp_interface *entry, char *ptr) {
	struct cdp_ipc_neighbor neighbor;

	/* make sure the neighbor is in a consistent state */
	memcpy(neighbor.interface, entry->name, sizeof(entry->name));
	memcpy(&neighbor.n, n, sizeof(struct cdp_neighbor));
	neighbor.n.ttl = neighbor.n.hnode->tstamp - time(NULL); 
	memcpy(ptr, &neighbor, sizeof(struct cdp_ipc_neighbor));
}

/**
 * Fetch cdp registered interfaces into the cdp response
 */
static int cdp_ipc_get_interfaces(char *cdpr, char *ifname) {
	struct cdp_interface *entry, *tmp;
	int count = 0;
	char *ptr = cdpr + sizeof(int);

	sys_dbg("requested interface: %s\n", ifname);
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		sem_wait(&entry->n_sem);
		if (ifname && strncmp(entry->name, ifname, strlen(ifname))) {
			sem_post(&entry->n_sem);
			continue;
		}
		count++;
		strncpy(ptr, entry->name, strlen(entry->name));
		ptr[strlen(entry->name)] = '\0';
		ptr += strlen(entry->name)+1;
		sem_post(&entry->n_sem);
	}

	return count;
}

/**
 * Fetch all known neighbors into the cdp response.
 */
static int cdp_ipc_get_neighbors(char *cdpr) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0;
	char *ptr = (char *) cdpr + sizeof(int);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		sem_wait(&entry->n_sem);
		list_for_each_entry_safe(n, t, &(entry->neighbors), lh) {
			cdp_ipc_add_neighbor(n, entry, ptr);
			ptr += sizeof(struct cdp_ipc_neighbor);
			count++;
			/* not enough memory for another neighbor in cdpr->data  */
			if (CDP_MAX_RESPONSE_SIZE-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
				sem_post(&entry->n_sem);
				return count;
			}
		}
		sem_post(&entry->n_sem);
	}
	return count;
}

/**
 * Fetch all known neighbors on the specified interface into the
 * cdp response.
 */
static int cdp_ipc_get_neighbors_intf(char *cdpr, char *interface) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0, found = 0;
	char *ptr = cdpr + sizeof(int);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (!strncmp(entry->name, interface, strlen(interface))) {
			found = 1;
			break;
		}

	if (!found)
		return 0;

	sem_wait(&entry->n_sem);
	list_for_each_entry_safe(n, t, &(entry->neighbors), lh) {
		cdp_ipc_add_neighbor(n, entry, ptr);	
		ptr += sizeof(struct cdp_ipc_neighbor);
		count++;
		/* not enough memory for another neighbor in cdpr->data  */
		if (CDP_MAX_RESPONSE_SIZE-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
			sem_post(&entry->n_sem);
			return count;
		}
	}
	sem_post(&entry->n_sem);
	return count;
}

/**
 * Fetch all neighbors with the specified device id into the
 * cdp response.
 */
static int cdp_ipc_get_neighbors_devid(char *cdpr, char *device_id) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0;
	char *ptr = (char *) cdpr + sizeof(int);

	sys_dbg("getting cdp entries with devid: %s\n", device_id);
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		sem_wait(&entry->n_sem);
		list_for_each_entry_safe(n, t, &(entry->neighbors), lh) {
			sys_dbg("analizing device: %s ... ", n->device_id);
			if (strncmp(n->device_id, device_id, strlen(device_id))) {
				sys_dbg("not matched\n");
				continue;
			}
			sys_dbg("matched");
			cdp_ipc_add_neighbor(n, entry, ptr);
			ptr += sizeof(struct cdp_ipc_neighbor);
			count++;
			/* not enough memory for another neighbor in cdpr->data  */
			if (CDP_MAX_RESPONSE_SIZE-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
				sem_post(&entry->n_sem);
				return count;
			}
		}
		sem_post(&entry->n_sem);
	}
	sys_dbg("%d matches for %s\n", count, device_id);
	return count;
}

static void cdp_ipc_show(struct cdp_show *sq, char *cdpr) {
	memset(cdpr, 0, CDP_MAX_RESPONSE_SIZE);
	sys_dbg("[ipc_listener]: ipc show query.\n");
	sys_dbg("[ipc listener]: show type: %d\n", sq->type);
	switch (sq->type) {
	case CDP_SHOW_CFG:
		memcpy(cdpr, &ccfg, sizeof(ccfg));
		break;
	case CDP_SHOW_STATS:
		memcpy(cdpr, &cdp_stats, sizeof(cdp_stats));
		break;
	case CDP_SHOW_INTF:
		*((int *) cdpr) = cdp_ipc_get_interfaces(cdpr, sq->interface);
		break;
	case CDP_SHOW_NEIGHBORS:
		sys_dbg("[ipc listener]: interface: %s, %d\n", sq->interface, strlen(sq->interface));
		sys_dbg("[ipc listener]: device_id: %s, %d\n", sq->device_id, strlen(sq->device_id));
		if (strlen(sq->interface))
			*((int *) cdpr) = cdp_ipc_get_neighbors_intf(cdpr, sq->interface);
		else if (strlen(sq->device_id))
			*((int *) cdpr) = cdp_ipc_get_neighbors_devid(cdpr, sq->device_id);
		else
			*((int *) cdpr) = cdp_ipc_get_neighbors(cdpr);
		break;
	}
}

static void cdp_ipc_conf(struct cdp_conf *cq, char *cdpr) {
	memset(cdpr, 0, CDP_MAX_RESPONSE_SIZE);
	sys_dbg("[ipc_listener]: ipc conf query.\n");
	switch (cq->field_id) {
	case CDP_CFG_VERSION:
		sys_dbg("[ipc_listener]: cdp advertise-v2 : %d\n", cq->field_value);
		if (cq->field_value >= 1 && cq->field_value <=2)
			ccfg.version = cq->field_value;
		break;
	case CDP_CFG_HOLDTIME:
		sys_dbg("[ipc listener]: cdp holdtime %d\n", cq->field_value);
		if (cq->field_value >= 10)
			ccfg.holdtime = cq->field_value;
		break;
	case CDP_CFG_TIMER:
		sys_dbg("[ipc listener]: cdp timer %d\n", cq->field_value);
		if (cq->field_value >= 5 && cq->field_value <= 254)
			ccfg.timer = cq->field_value;
		break;
	case CDP_CFG_ENABLED:
		sys_dbg("[ipc listener]: cdp enabled %d\n", cq->field_value);
		if (cq->field_value <= 1)
			ccfg.enabled = cq->field_value;
		break;
	default:
		sys_dbg("[ipc listener]: invalid configuration field id: %d\n", cq->field_id);
	}
}

static void cdp_ipc_adm(struct cdp_adm *aq, char *cdpr) {
	memset(cdpr, 0, CDP_MAX_RESPONSE_SIZE);
	sys_dbg("[ipc listener]: ipc adm query.\n");
	switch (aq->type) {
	case CDP_IF_ENABLE:
		sys_dbg("[ipc listener]: Enable cdp on interface %s\n", aq->interface);
		register_cdp_interface(aq->interface);
		break;
	case CDP_IF_DISABLE:
		sys_dbg("[ipc listener]: Disable cdp on interface %s\n", aq->interface);
		unregister_cdp_interface(aq->interface);
		break;
	case CDP_IF_STATUS:
		sys_dbg("[ipc listener]: Get cdp status on interface %s\n", aq->interface);
		*((int *) cdpr) = get_cdp_status(aq->interface);
		break;
	default:
		sys_dbg("[ipc listener]: Unknown administrative command type %d.\n", aq->type);
	}
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

		switch (m->type) {
		case CDP_SHOW_QUERY:
			cdp_ipc_show(&m->query.show, r);
			break;
		case CDP_CONF_QUERY:
			cdp_ipc_conf(&m->query.conf, r);
			break;
		case CDP_ADM_QUERY:
			cdp_ipc_adm(&m->query.adm, r);
			break;
		}

		/* send the response */
		cdp_ipc_send_response(m, r);
	}

	free(msg);
	pthread_exit(NULL);
}

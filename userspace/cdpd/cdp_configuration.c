#include "cdpd.h"
#include "cdp_ipc.h"
#include "debug.h"

/**
 * Configuration management via a IPC message queueue.
 */
extern struct cdp_configuration cfg;
extern struct list_head registered_interfaces;
extern struct cdp_traffic_stats cdp_stats;

extern void register_cdp_interface(char *);
extern void unregister_cdp_interface(char *);
extern int get_cdp_status(char *);

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
static int cdp_ipc_get_interfaces(struct cdp_response *cdpr, char *ifname) {
	struct cdp_interface *entry, *tmp;
	int count = 0;
	char *ptr = (char *) cdpr + sizeof(int);

	dbg("requested interface: %s\n", ifname);
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
static int cdp_ipc_get_neighbors(struct cdp_response *cdpr) {
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
			if (sizeof(cdpr->data)-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
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
static int cdp_ipc_get_neighbors_intf(struct cdp_response *cdpr, char *interface) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0, found = 0;
	char *ptr = (char *) cdpr + sizeof(int);

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
		if (sizeof(cdpr->data)-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
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
static int cdp_ipc_get_neighbors_devid(struct cdp_response *cdpr, char *device_id) {
	struct cdp_interface *entry, *tmp;
	struct cdp_neighbor *n, *t;
	int count = 0;
	char *ptr = (char *) cdpr + sizeof(int);

	dbg("getting cdp entries with devid: %s\n", device_id);
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		sem_wait(&entry->n_sem);
		list_for_each_entry_safe(n, t, &(entry->neighbors), lh) {
			dbg("analizing device: %s ... ", n->device_id);
			if (strncmp(n->device_id, device_id, strlen(device_id))) {
				dbg("not matched\n");
				continue;
			}
			dbg("matched");
			cdp_ipc_add_neighbor(n, entry, ptr);
			ptr += sizeof(struct cdp_ipc_neighbor);
			count++;
			/* not enough memory for another neighbor in cdpr->data  */
			if (sizeof(cdpr->data)-sizeof(int) < (count+1) * sizeof(struct cdp_ipc_neighbor)) {
				sem_post(&entry->n_sem);
				return count;
			}
		}
		sem_post(&entry->n_sem);
	}
	dbg("%d matches for %s\n", count, device_id);
	return count;
}

static void cdp_ipc_show(struct cdp_ipc_message *m, struct cdp_ipc_message *r) {
	struct cdp_show_query *sq;
	struct cdp_response *cdpr;

	/* response type is the sender's pid */
	sq = (struct cdp_show_query *) (m->buf+sizeof(pid_t));
	cdpr = (struct cdp_response *) r->buf;
	memset(cdpr->data, 0, sizeof(cdpr->data));
	dbg("[ipc_listener]: ipc show query.\n");
	dbg("[ipc listener]: response type: %d\n", r->type);
	dbg("[ipc listener]: show type: %d\n", sq->show_type);
	switch (sq->show_type) {
	case CDP_IPC_SHOW_CFG: 
		memcpy(cdpr->data, &cfg, sizeof(cfg)); 
		break;
	case CDP_IPC_SHOW_STATS:
		memcpy(cdpr->data, &cdp_stats, sizeof(cdp_stats));
		break;
	case CDP_IPC_SHOW_INTF:
		*((int *) cdpr) = cdp_ipc_get_interfaces(cdpr, sq->interface);
		break;
	case CDP_IPC_SHOW_NEIGHBORS:
		dbg("[ipc listener]: interface: %s, %d\n", sq->interface, strlen(sq->interface));
		dbg("[ipc listener]: device_id: %s, %d\n", sq->device_id, strlen(sq->device_id));
		if (strlen(sq->interface))
			*((int *) cdpr) = cdp_ipc_get_neighbors_intf(cdpr, sq->interface);
		else if (strlen(sq->device_id))
			*((int *) cdpr) = cdp_ipc_get_neighbors_devid(cdpr, sq->device_id);
		else
			*((int *) cdpr) = cdp_ipc_get_neighbors(cdpr);
		break;
	}
}

static void cdp_ipc_conf(struct cdp_ipc_message *m, struct cdp_ipc_message *r) {
	struct cdp_conf_query *cq;
	struct cdp_response *cdpr;

	cq = (struct cdp_conf_query *) (m->buf + sizeof(pid_t));
	cdpr = (struct cdp_response *) r->buf;
	memset(cdpr->data, 0, sizeof(cdpr->data));
	dbg("[ipc_listener]: ipc conf query.\n");
	switch (cq->field_id) {
	case CDP_IPC_CFG_VERSION:
		dbg("[ipc_listener]: cdp advertise-v2 : %d\n", cq->field_value);
		if (cq->field_value >= 1 && cq->field_value <=2)
			cfg.version = cq->field_value;
		break;
	case CDP_IPC_CFG_HOLDTIME:
		dbg("[ipc listener]: cdp holdtime %d\n", cq->field_value);
		if (cq->field_value >= 10)
			cfg.holdtime = cq->field_value;
		break;
	case CDP_IPC_CFG_TIMER:
		dbg("[ipc listener]: cdp timer %d\n", cq->field_value);
		if (cq->field_value >= 5 && cq->field_value <= 254)
			cfg.timer = cq->field_value;
		break;
	case CDP_IPC_CFG_ENABLED:
		dbg("[ipc listener]: cdp enabled %d\n", cq->field_value);
		if (cq->field_value <= 1)
			cfg.enabled = cq->field_value;
		break;
	default:
		dbg("[ipc listener]: invalid configuration field id: %d\n", cq->field_id);
	}
}

static void cdp_ipc_adm(struct cdp_ipc_message *m, struct cdp_ipc_message *r) {
	struct cdp_adm_query *aq;
	struct cdp_response *cdpr;

	aq = (struct cdp_adm_query *) (m->buf + sizeof(pid_t));
	cdpr = (struct cdp_response *) r->buf;
	memset(cdpr->data, 0, sizeof(cdpr->data));
	dbg("[ipc listener]: ipc adm query.\n");
	switch (aq->type) {
	case CDP_IPC_IF_ENABLE:
		dbg("[ipc listener]: Enable cdp on interface %s\n", aq->interface);
		register_cdp_interface(aq->interface);
		break;
	case CDP_IPC_IF_DISABLE:
		dbg("[ipc listener]: Disable cdp on interface %s\n", aq->interface);
		unregister_cdp_interface(aq->interface);
		break;
	case CDP_IPC_IF_STATUS:
		dbg("[ipc listener]: Get cdp status on interface %s\n", aq->interface);
		*((int *) cdpr) = get_cdp_status(aq->interface);
		break;
	default:
		dbg("[ipc listener]: Unknown administrative command type %d.\n", aq->type);
	}
}

void *cdp_ipc_listen(void *arg) {
	int qid, s;
	struct cdp_ipc_message m, r;

	if ((qid = msgget(CDP_IPC_QUEUE_KEY, IPC_CREAT|0666)) == -1) {
		perror("msgget");
		exit(-1);
	}

	dbg("cdp ipc listen\n");
	for (;;) {
		/* receive a message from the message queue */
		if ((s = msgrcv(qid, &m, sizeof(struct cdp_ipc_message), 0, 0)) < 0) {
			perror("msgrcv");
			continue;
		}
		/* interpret & compose response */
		dbg("[ipc listener]: received message of type: %d\n", m.type);
		dbg("[ipc listener]: sender pid is: %d\n", *((pid_t *)m.buf));
		/* response type is the sender's pid */
		r.type = *((pid_t *)m.buf);
		switch (m.type) {
		case CDP_IPC_SHOW_QUERY:
			cdp_ipc_show(&m, &r);
			break;
		case CDP_IPC_CONF_QUERY:
			cdp_ipc_conf(&m, &r);
			break;
		case CDP_IPC_ADM_QUERY:
			cdp_ipc_adm(&m, &r);
			break;
		}
		/* send the response */
		s = msgsnd(qid, &r, sizeof(struct cdp_ipc_message), IPC_NOWAIT);
		dbg("sent response, result: %d\n", s);
	}
	pthread_exit(NULL);
}

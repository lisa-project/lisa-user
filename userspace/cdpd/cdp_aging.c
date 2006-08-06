#include "cdpd.h"
#include "debug.h"

/* cdp neighbor heap (used for aging mechanism) */
neighbor_heap_t *nheap;
int hend, heap_size;
sem_t nheap_sem;

/* Exchange 2 elements in the heap */
#define _XCHG(h, i1, i2) do { \
		neighbor_heap_t temp; \
		temp = h[i1]; \
		h[i1] = h[i2]; \
		h[i1].n->hnode = &h[i1]; \
		h[i2] = temp; \
		h[i2].n->hnode = &h[i2]; \
	} while(0);

/**
 * Sift up an element in the neighbor heap.
 */
void sift_up(neighbor_heap_t *heap, int pos) {
	while (pos > 0) {
		if (heap[pos].tstamp >= heap[PARENT(pos)].tstamp)
			return;
		/* swap node with parent */
		_XCHG(heap, pos, PARENT(pos));
		/* continue with parent */
		pos = PARENT(pos);
	}
}

/**
 * Sift down an element in the neighbor heap.
 */
void sift_down(neighbor_heap_t *heap, int pos, int heap_end) {
	int min;

	while (pos < heap_end) {
		/* no more elements to process */
		if (LEFT(pos) > heap_end)
			return;
		/* assume min is the index of the left child */
		min = LEFT(pos);
		/* if we have a child on the right and its tstamp is less than the timestamp
		 at index min, set min to its index */
		if (RIGHT(pos) <= heap_end && heap[RIGHT(pos)].tstamp < heap[min].tstamp)
			min = RIGHT(pos);

		if (heap[pos].tstamp <= heap[min].tstamp)
			return;

		/* swap pos with min */
		_XCHG(heap, min, pos);

		/* continue with index min */
		pos = min;
	}
}

/* every second check for expired entries and clean them*/
void *cdp_clean_loop(void *arg) {
	struct cdp_neighbor *n;
	struct cdp_interface *i;

	for (;;) {
		sleep(1);
		dbg("[cdp clean loop]\n");
		/* if no neighbor was registered we continue */
		if (hend < 0)
			continue;
		/* clean expired neighbor entries */
		while (ROOT(nheap).tstamp <= time(NULL)) {
			sem_wait(&nheap_sem);
			/* if an update to the root node was received while we were waiting
			at the semaphore, we must avoid the race condition */
			if (ROOT(nheap).tstamp > time(NULL)) {
				sem_post(&nheap_sem);
				break;
			}
			n = ROOT(nheap).n;
			i = n->interface;
			dbg("[cdp cleaner]: cleaning cdp neighbor %s\n", n->device_id);
			/* get the lock on the interface neighbor list */
			sem_wait(&i->n_sem);
			/* safely delete the expired neighbor */
			list_del(&n->lh);
			free(n);
			/* release the lock on the interface neighbor list */
			sem_post(&i->n_sem);
			ROOT(nheap) = nheap[hend];
			hend--;
			/* if the heap is empty, we continue */
			if (hend < 0) {
				sem_post(&nheap_sem);
				break;
			}
			sift_down(nheap, 0, hend);
			sem_post(&nheap_sem);
		}
	}
}

#include "cdpd.h"
#include "debug.h"

/* cdp neighbor heap (used for aging mechanism) */
neighbor_heap_t *nheap;
int hend, heap_size;
sem_t nheap_sem;

/**
 * Sift up an element in the neighbor heap.
 */
void sift_up(neighbor_heap_t *heap, int pos) {
	neighbor_heap_t temp;

	while (pos > 0) {
		if (heap[pos].tstamp >= heap[PARENT(pos)].tstamp)
			return;
		/* swap node with parent */
		temp = heap[pos];
		heap[pos] = heap[PARENT(pos)];
		heap[PARENT(pos)] = temp;
		/* continue with parent */
		pos = PARENT(pos);
	}
}

/**
 * Sift down an element in the neighbor heap.
 */
void sift_down(neighbor_heap_t *heap, int pos, int heap_end) {
	neighbor_heap_t temp;
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
		temp = heap[min];
		heap[min] = heap[pos];
		heap[pos] = temp;

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
			/* the condition is true even if we waited at the semaphore and other elements were
			 inserted into the heap (the heap is a min-heap and this is the only place we're 
			 extracting from it) */
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

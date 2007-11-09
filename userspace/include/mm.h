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

#ifndef _MM_H
#define _MM_H

#include <semaphore.h>

typedef unsigned int mm_ptr_t;
#define MM_NULL 0

#define MM_STATIC(mm) ((void *)((mm)->area + sizeof(struct mm_shared)))

/* mm managed area handle */
struct mm_private {
	int fd;				/* file descriptor of posix shm area */
	sem_t *sem;			/* posix semaphore for shm area synchronization */
	int lock;			/* mm_lock() counter */
	char *area;			/* base pointer of mmap'ed pages */
	size_t mapped_size;	/* total size of mmap'ed pages */
	int init;			/* whether mm_create() created (1) or opened (0) */
};

/* kernel style list for mm */
struct mm_list_head {
	mm_ptr_t prev, next;
};

/* main descriptor of shared memory area */
struct mm_shared {
	size_t static_size;
	size_t dynamic_size;
	struct mm_list_head lh;
};

struct mm_chunk {
	/* chunk usable size (w/o sizeof(struct mm_chunk)) */
	size_t size;
	struct mm_list_head lh;
};

static __inline__ void *mm_addr(struct mm_private *mm, mm_ptr_t p)
{
	return MM_NULL == p ? NULL : (void *)(mm->area + p);
}

static __inline__ mm_ptr_t mm_ptr(struct mm_private *mm, void *a)
{
	return NULL == a ? MM_NULL : (char *)a - mm->area;
}

static __inline__ void __mm_lock(struct mm_private *mm)
{
	if (mm->lock++)
		return;
	sem_wait(mm->sem);
}

#define MM_INIT_LIST_HEAD(mm, ptr) do { \
	MM_LHPTR(mm, ptr)->next = (ptr); \
	MM_LHPTR(mm, ptr)->prev = (ptr); \
} while (0)

#define mm_list_entry(ptr, type, member) \
	((mm_ptr_t)((ptr) - (unsigned long)(&((type *)0)->member)))

#define MM_LHPTR(mm, p) ((struct mm_list_head *)mm_addr((mm), (p)))

extern void mm_lock(struct mm_private *);

static __inline__ void mm_unlock(struct mm_private *mm)
{
	if (--mm->lock)
		return;
	sem_post(mm->sem);
}

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __mm_list_add(struct mm_private *mm, mm_ptr_t new,
		mm_ptr_t prev, mm_ptr_t next)
{
	MM_LHPTR(mm, next)->prev = new;
	MM_LHPTR(mm, new)->next = next;
	MM_LHPTR(mm, new)->prev = prev;
	MM_LHPTR(mm, prev)->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void mm_list_add(struct mm_private *mm, mm_ptr_t new,
		mm_ptr_t head)
{
	__mm_list_add(mm, new, head, MM_LHPTR(mm, head)->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void mm_list_add_tail(struct mm_private *mm, mm_ptr_t new,
		mm_ptr_t head)
{
	__mm_list_add(mm, new, MM_LHPTR(mm, head)->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __mm_list_del(struct mm_private *mm, mm_ptr_t prev,
		mm_ptr_t next)
{
	MM_LHPTR(mm, next)->prev = prev;
	MM_LHPTR(mm, prev)->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static __inline__ void mm_list_del(struct mm_private *mm, mm_ptr_t entry)
{
	__mm_list_del(mm, MM_LHPTR(mm, entry)->prev, MM_LHPTR(mm, entry)->next);
	MM_LHPTR(mm, entry)->next = MM_LHPTR(mm, entry)->prev = 0;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline__ void mm_list_del_init(struct mm_private *mm, mm_ptr_t entry)
{
	__mm_list_del(mm, MM_LHPTR(mm, entry)->prev, MM_LHPTR(mm, entry)->next);
	MM_INIT_LIST_HEAD(mm, entry); 
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline__ int mm_list_empty(struct mm_private *mm, mm_ptr_t head)
{
	return MM_LHPTR(mm, head)->next == head;
}

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define mm_list_for_each(mm, pos, head) \
	for (pos = MM_LHPTR(mm, head)->next; pos != (head); \
        	pos = MM_LHPTR(mm, pos)->next)

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define mm_list_for_each_safe(mm, pos, n, head) \
	for (pos = MM_LHPTR(mm, head)->next, n = MM_LHPTR(mm, pos)->next; pos != (head); \
		pos = n, n = MM_LHPTR(mm, pos)->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define mm_list_for_each_entry(mm, pos, head, member)				\
	for (pos = mm_addr(mm, mm_list_entry(MM_LHPTR(mm, head)->next, typeof(*pos), member));	\
	     &pos->member != mm_addr(mm, head); 		\
	     pos = mm_addr(mm, mm_list_entry(pos->member.next, typeof(*pos), member)))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define mm_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = mm_addr(mm, mm_list_entry(MM_LHPTR(mm, head)->next, typeof(*pos), member)),	\
		n = mm_addr(mm, mm_list_entry(pos->member.next, typeof(*pos), member));	\
	     &pos->member != mm_addr(mm, head); 					\
	     pos = n, n = mm_addr(mm, mm_list_entry(pos->member.next, typeof(*pos), member)))

extern struct mm_private *mm_create(const char *, size_t, size_t);
extern mm_ptr_t mm_alloc(struct mm_private *, size_t);
extern void mm_free(struct mm_private *, mm_ptr_t);
extern mm_ptr_t mm_realloc(struct mm_private *, mm_ptr_t, size_t);

#endif

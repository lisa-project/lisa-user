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

typedef mm_ptr unsigned int;
typedef mm_dptr unsigned int;

/* mm managed area handle */
struct mm_private {
	int fd;
	sem_t *sem;
	char *area;
	char *dynamic_area;
}

/* kernel style list for mm */
struct mm_list_head {
	mm_ptr prev, next;
}

/* main descriptor of shared memory area */
struct mm_shared {
	size_t static_size;
	size_t dynamic_size;
	struct mm_list_head lh;
}

struct mm_chunk {
	/* chunk usable size (w/o sizeof(struct mm_chunk)) */
	unsigned int size;
	int free;
	struct mm_list_head lh;
}

static __inline__ void *mm_addr(struct mm_private *mm, mm_ptr p)
{
	return (void *)(mm->dynamic_area + p);
}

#define MM_INIT_LIST_HEAD(mm, ptr) do { \
	(struct mm_list_head *)mm_addr(mm, ptr)->next = (ptr); \
	(struct mm_list_head *)mm_addr(mm, ptr)->prev = (ptr); \
} while (0)

#define mm_list_entry(ptr, type, member) \
	((mm_ptr)((ptr) - (unsigned long)(&((type *)0)->member)))

static __inline__ void mm_lock(struct mm_private *mm)
{
	sem_wait(mm->sem);
}

static __inline__ void mm_unlock(struct mm_private *mm)
{
	sem_post(mm->sem);
}

#endif

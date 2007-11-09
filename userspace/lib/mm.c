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

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mm.h"

static __inline__ void *mm_map(struct mm_private *mm)
{
	return mmap(NULL, mm->mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, mm->fd, 0);
}

static __inline__ size_t page_size(size_t size)
{
	size_t p_size = sysconf(_SC_PAGE_SIZE);
	return p_size * ((size + p_size - 1) / p_size);
}

struct mm_private *mm_create(const char *name, size_t static_size, size_t dynamic_size)
{
	struct mm_private *ret;
	int unlock = 0, unlink = 0; /* cleanup flags */

	ret = malloc(sizeof(struct mm_private));
	if (NULL == ret)
		return NULL;

	ret->fd = -1;
	ret->lock = 0;
	do {
		ret->sem = sem_open(name, O_CREAT, 0600, 1);
		if (SEM_FAILED == ret->sem)
			break;

		__mm_lock(ret);
		unlock = 1;
		ret->fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);

		if (-1 == ret->fd && EEXIST != errno)
			break;

		if (-1 == ret->fd) {
			/* already there, open without O_EXCL and go; if
			 * something goes wrong, close and cleanup */
			ret->fd = shm_open(name, O_RDWR, 0600);
			ret->mapped_size = sizeof(struct mm_shared);
			if (-1 == ret->fd)
				break;
			ret->area = mm_map(ret);
			if (NULL == ret->area)
				break;
			ret->mapped_size = sizeof(struct mm_shared) +
				((struct mm_shared *)ret->area)->static_size +
				((struct mm_shared *)ret->area)->dynamic_size;
			munmap(ret->area, sizeof(struct mm_shared));
			ret->area = mm_map(ret);
			if (NULL == ret->area)
				break;
			ret->init = 0;
		} else {
			/* we just created the shm area, must setup; if
			 * something goes wrong, delete the shm area and
			 * cleanup (don't leave half-initialized garbage) */
			ret->mapped_size = page_size(static_size + dynamic_size + sizeof(struct mm_shared));

			unlink = 1;

			if (-1 == ftruncate(ret->fd, ret->mapped_size))
				break;
			ret->area = mm_map(ret);
			if (NULL == ret->area)
				break;

			/* initialize shared data and dynamic allocator */
			((struct mm_shared *)ret->area)->static_size = static_size;
			((struct mm_shared *)ret->area)->dynamic_size = ret->mapped_size -
				(sizeof(struct mm_shared) + static_size);
			MM_INIT_LIST_HEAD(ret, mm_ptr(ret, &((struct mm_shared *)ret->area)->lh));
			ret->init = 1;
		}

		mm_unlock(ret);
		return ret;
	} while(0);

	/* cleanup */
	if (-1 != ret->fd)
		close(ret->fd);

	if (unlink)
		shm_unlink(name);

	if (unlock)
		mm_unlock(ret);
	
	free(ret);
	return NULL;
}

/* Lock the associated semaphore and make sure the shm area is
 * properly mapped (i.e. the shm size has not changed)
 */
void mm_lock(struct mm_private *mm)
{
	size_t mm_size;

	if (mm->lock++)
		return;
	sem_wait(mm->sem);

	mm_size = sizeof(struct mm_shared) +
		((struct mm_shared *)mm->area)->static_size +
		((struct mm_shared *)mm->area)->dynamic_size;
 
	if (mm_size <= mm->mapped_size)
		return;

	munmap(mm->area, mm->mapped_size);
	mm->mapped_size = mm_size;
	mm->area = mm_map(mm);
}

/* Extend the mm area by size bytes */
int mm_extend(struct mm_private *mm, size_t size)
{
	size_t mm_size;
	if (!size)
		return 0;

	mm_lock(mm);
	mm_size = mm->mapped_size + size;
	if (-1 == ftruncate(mm->fd, mm_size)) {
		mm_unlock(mm);
		return 1;
	}

	munmap(mm->area, mm->mapped_size);
	mm->mapped_size = mm_size;
	mm->area = mm_map(mm);

	mm_unlock(mm);
	return 0;
}

mm_ptr_t mm_alloc(struct mm_private *mm, size_t size)
{
	mm_ptr_t base, last;
	size_t real_size;
	struct mm_chunk *chunk;
	struct mm_shared *shr;

	if (!size)
		return MM_NULL;

	mm_lock(mm);
	shr = (struct mm_shared *)mm->area;
	last = base = sizeof(struct mm_shared) + shr->static_size;
	real_size = sizeof(struct mm_chunk) + size;

	do {
		/* first see if there is no allocated chunk yet */
		if (mm_list_empty(mm, mm_ptr(mm, &shr->lh)))
			break;

		/* find first gap that can accomodate */
		mm_list_for_each_entry(mm, chunk, mm_ptr(mm, &shr->lh), lh) {
			if (mm_ptr(mm, chunk) - last < real_size) {
				last = mm_ptr(mm, chunk) + chunk->size;
				continue;
			}
			chunk = mm_addr(mm, last);
			chunk->size = real_size;
			__mm_list_add(mm, mm_ptr(mm, &chunk->lh), chunk->lh.prev, chunk->lh.next);
			mm_unlock(mm);
			return last + sizeof(struct mm_chunk);
		}
	
		/* allocate between the last chunk and the end of the dynamic area;
		 * last is already set up properly */
	} while (0);

	if (base + shr->dynamic_size < last + real_size)
		if (mm_extend(mm, (last + real_size) - (base + shr->dynamic_size))) {
			mm_unlock(mm);
			return MM_NULL;
		}
	chunk = mm_addr(mm, last);
	chunk->size = real_size;
	mm_list_add_tail(mm, mm_ptr(mm, &chunk->lh), mm_ptr(mm, &shr->lh));
	mm_unlock(mm);
	return last + sizeof(struct mm_chunk);
	/* don't use mm_ptr() on chunk, because we know it's at last ;) */
}

void mm_free(struct mm_private *mm, mm_ptr_t ptr)
{
	if (MM_NULL == ptr)
		return;
	mm_lock(mm);
	mm_list_del(mm, mm_ptr(mm, &((struct mm_chunk *)mm_addr(mm, ptr - sizeof(struct mm_chunk)))->lh));
	mm_unlock(mm);
}

mm_ptr_t mm_realloc(struct mm_private *mm, mm_ptr_t ptr, size_t size)
{
	struct mm_chunk *chunk;
	size_t real_size;
	mm_ptr_t new;

	if (MM_NULL == ptr)
		return mm_alloc(mm, size);
	if (!size) {
		mm_free(mm, ptr);
		return MM_NULL;
	}

	mm_lock(mm);
	chunk = mm_addr(mm, ptr - sizeof(struct mm_chunk));
	real_size = size + sizeof(struct mm_chunk);

	if (chunk->size >= real_size) {
		chunk->size = real_size;
		mm_unlock(mm);
		return ptr;
	}

	new = mm_alloc(mm, size);
	if (MM_NULL == new) {
		mm_unlock(mm);
		return MM_NULL;
	}
	memcpy(mm_addr(mm, new), mm_addr(mm, ptr), chunk->size);
	mm_free(mm, ptr);
	mm_unlock(mm);
	return new;
}

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

struct mm_private *mm_create(const char *name, size_t static_size, size_t dynamic_size) {
	struct mm_private *ret;
	int unlock = 0;
	void *mm_area;

	ret = malloc(sizeof(struct mm_private));
	if (NULL == ret)
		return NULL;
	ret->fd = -1;

	do {
		ret->sem = sem_open(name, O_CREAT, 0600, 1);
		if (SEM_FAILED == ret->sem)
			break;

		mm_lock(ret);
		unlock = 1;
		ret->fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);

		if (-1 == ret->fd && EEXIST != errno)
			break;

		if (-1 == ret->fd) {
			/* already there, open without O_EXCL and go; if
			 * something goes wrong, close and cleanup */
			ret->fd = shm_open(name, O_RDWR, 0600);
			if (-1 == ret->fd)
				break;
			mm_area = mmap(NULL, sizeof(struct mm_shared), PROT_READ | PROT_WRITE, ret->fd, 0);
			if (NULL == mm_area)
		} else {
			/* we just created the shm area, must setup; if
			 * something goes wrong, delete the shm area and
			 * cleanup (don't leave half-initialized garbage) */
		}


		mm_unlock(ret);
	} while(0);

	if (unlock)
		mm_unlock(ret);
	
	free(ret);
	return NULL;

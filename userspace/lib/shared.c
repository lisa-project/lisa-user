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

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "mm.h"
#include "shared.h"

/*
 * Switch shared memory structure
 */
struct shared {
	/* Enable secrets (crypted) */
	struct {
		char secret[SW_SECRET_LEN + 1];
	} enable[SW_MAX_ENABLE+1];
	/* Line vty passwords (clear text) */
	struct {
		char passwd[SW_PASS_LEN + 1];
	} vty[SW_MAX_VTY + 1];
	/* CDP configuration */
	struct cdp_configuration cdp;
	/* List of interface tags */
	struct mm_list_head if_tags;
};

struct if_tag {
	int if_index;
	char tag[SW_MAX_TAG + 1];
	struct mm_list_head lh;
};

static struct mm_private *mm = NULL;
#define SHM ((struct shared *)MM_STATIC(mm))

static mm_ptr_t __shared_get_if_tag(int if_index) {
	mm_ptr_t ret;

	mm_list_for_each(mm, ret, mm_ptr(mm, &SHM->if_tags)) {
		struct if_tag *tag =
			mm_addr(mm, mm_list_entry(ret, struct if_tag, lh));
		if (tag->if_index == if_index)
			return ret;
	}

	return MM_NULL;
}

static int __shared_del_if_tag(int if_index) {
	mm_ptr_t lh = __shared_get_if_tag(if_index);

	if (MM_NULL == lh)
		return 1;
	mm_list_del(mm, lh);
	mm_free(mm, mm_list_entry(lh, struct if_tag, lh));
	return 0;
}

static mm_ptr_t __shared_get_tag_if(char *tag) {
	mm_ptr_t ret;

	mm_list_for_each(mm, ret, mm_ptr(mm, &SHM->if_tags)) {
		struct if_tag *s_tag =
			mm_addr(mm, mm_list_entry(ret, struct if_tag, lh));
		if (!strcmp(tag, s_tag->tag))
			return ret;
	}

	return MM_NULL;
}

static void shared_init_cdp(void)
{
	struct cdp_configuration cdp;

	/* Initial default values for the cdp configuration */
	cdp.enabled  = 1;                /* CDP is enabled by default */
	cdp.version  = CDP_DFL_VERSION;  /* CDPv2*/
	cdp.holdtime = CDP_DFL_HOLDTIME; /* 180 seconds */
	cdp.timer    = CDP_DFL_TIMER;    /* 60 seconds */

	/* store initial config into the shared memory */
	shared_set_cdp(&cdp);
}

int shared_init(void) {
	if (mm)
		return 0;

	mm = mm_create("lisa", sizeof(struct shared), 4096);
	if (!mm)
		return  -1;

	if (mm->init) {
		memset(SHM, 0, sizeof(struct shared));
		MM_INIT_LIST_HEAD(mm, mm_ptr(mm, &SHM->if_tags));
		shared_init_cdp();
	}

	return 0;
}

int shared_auth(int type, int level,
		int (*auth)(char *pw, void *priv), void *priv)
{
	char *passwd;
	int err = -EINVAL;

	if (!auth)
		return err;

	mm_lock(mm);
	switch (type) {
	case SHARED_AUTH_VTY:
		if (level < 0 || level > SW_MAX_VTY)
			goto out_unlock;
		passwd = SHM->vty[level].passwd;
		break;
	case SHARED_AUTH_ENABLE:
		if (level < 0 || level > SW_MAX_ENABLE)
			goto out_unlock;
		passwd = SHM->enable[level].secret;
		break;
	default:
		goto out_unlock;
		break;
	}
	err = auth(passwd, priv);

out_unlock:
	mm_unlock(mm);

	return err;
}

int shared_set_passwd(int type, int level, char *passwd)
{
	int err = -EINVAL;

	if (!passwd)
		return err;

	mm_lock(mm);
	switch (type) {
	case SHARED_AUTH_VTY:
		if (level < 0 || level > SW_MAX_VTY)
			goto out_unlock;
		strncpy(SHM->vty[level].passwd, passwd, SW_PASS_LEN);
		SHM->vty[level].passwd[SW_PASS_LEN] = 0;
		err = 0;
		break;
	case SHARED_AUTH_ENABLE:
		if (level < 0 || level > SW_MAX_ENABLE)
			goto out_unlock;
		strncpy(SHM->enable[level].secret, passwd, SW_SECRET_LEN);
		SHM->enable[level].secret[SW_SECRET_LEN] = 0;
		err = 0;
		break;
	default:
		break;
	}

out_unlock:
	mm_unlock(mm);

	return err;
}

int shared_get_if_tag(int if_index, char *tag) {
	mm_ptr_t ptr;
	struct if_tag *s_tag;

	mm_lock(mm);
	ptr = __shared_get_if_tag(if_index);
	if (MM_NULL == ptr) {
		mm_unlock(mm);
		return 1;
	}
	s_tag = mm_addr(mm, mm_list_entry(ptr, struct if_tag, lh));
	strcpy(tag, s_tag->tag);

	mm_unlock(mm);
	return 0;
}


int shared_set_if_tag(int if_index, char *tag, int *other_if) {
	mm_ptr_t lh, mm_s_tag;
	struct if_tag *s_tag;
	int ret;

	mm_lock(mm);

	if (NULL == tag) {
		ret = __shared_del_if_tag(if_index);
		mm_unlock(mm);
		return ret;
	}

	lh = __shared_get_tag_if(tag);
	if (MM_NULL != lh) {
		if (*other_if) {
			s_tag = mm_addr(mm, mm_list_entry(lh, struct if_tag, lh));
			s_tag->if_index = *other_if;
		}
		mm_unlock(mm);
		return 1;
	}

	lh = __shared_get_if_tag(if_index);
	if (MM_NULL != lh) {
		s_tag = mm_addr(mm, mm_list_entry(lh, struct if_tag, lh));
		strncpy(s_tag->tag, tag, SW_MAX_TAG);
		s_tag->tag[SW_MAX_TAG] = '\0';
		mm_unlock(mm);
		return 0;
	}

	mm_s_tag = mm_alloc(mm, sizeof(struct if_tag));
	/* first save mm pointer obtained from mm_alloc, then compute s_tag
	 * pointer, because mm_alloc() can change mm->area if the shm area
	 * is extended (refer to README.mm for details) */
	s_tag = mm_addr(mm, mm_s_tag);
	if (NULL == s_tag) {
		if (*other_if)
			*other_if = 0;
		mm_unlock(mm);
		return 1;
	}

	s_tag->if_index = if_index;
	strncpy(s_tag->tag, tag, SW_MAX_TAG);
	s_tag->tag[SW_MAX_TAG] = '\0';
	mm_list_add(mm, mm_ptr(mm, &s_tag->lh), mm_ptr(mm, &SHM->if_tags));

	mm_unlock(mm);
	return 0;
}

void shared_set_cdp(struct cdp_configuration *cdp)
{
	assert(cdp);
	mm_lock(mm);
	memcpy(&SHM->cdp, cdp, sizeof(struct cdp_configuration));
	mm_unlock(mm);
}

void shared_get_cdp(struct cdp_configuration *cdp)
{
	assert(cdp);
	mm_lock(mm);
	memcpy(cdp, &SHM->cdp, sizeof(struct cdp_configuration));
	mm_unlock(mm);
}

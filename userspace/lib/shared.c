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

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "shared.h"

/* the following need to be moved to shared.h as soon as we figure
 * out how to fix the kernel/userspace headers mixing crap
 */
#define IFNAMSIZ 16
/* arghhhh this is really nasty */

struct if_tag {
	char if_name[IFNAMSIZ];
	char tag[CLI_MAX_TAG + 1];
	struct mm_list_head lh;
};
/* end of things to be moved */

extern int errno;

struct mm_private *cfg = NULL;

void cfg_init_data(void) {
	memset(CFG, 0, sizeof(struct cli_config));
	MM_INIT_LIST_HEAD(cfg, mm_ptr(cfg, &CFG->if_tags));
}

int cfg_init(void) {
	if (cfg)
		return 0;

	cfg = mm_create("lisa", sizeof(struct cli_config), 4096);
	if (!cfg)
		return -1;

	if (cfg->init)
		cfg_init_data();

	return 0;
}

static void sw_redisplay_password(void) {
	fprintf(rl_outstream, "\rPassword: ");
	fflush(rl_outstream);
}

int cfg_checkpass(int retries, int (*validator)(char *, void *), void *arg) {
	rl_voidfunc_t *old_redisplay = rl_redisplay_function;
	char *pw;
	int i;

	rl_redisplay_function = sw_redisplay_password;
	for(i = 0; i < retries; i++) {
		pw = readline(NULL);
		if(validator(pw, arg))
			break;
	}
	rl_redisplay_function = old_redisplay;
	return i < retries;
}

static void sw_redisplay_void(void) {
}

int cfg_waitcr(void) {
	rl_voidfunc_t *old_redisplay = rl_redisplay_function;

	rl_redisplay_function = sw_redisplay_void;
	readline(NULL);
	rl_redisplay_function = old_redisplay;
	return 0;
}

int read_key() {
	int ret;
	struct termios t_old, t_new;

	tcgetattr(0, &t_old);
	t_new = t_old;
	t_new.c_lflag = ~ICANON;
	t_new.c_cc[VTIME] = 0;
	t_new.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &t_new);
	ret = getchar();
	tcsetattr(0, TCSANOW, &t_old);
	return ret;
}

const char config_file[] = "/etc/lisa/config.text";
const char config_tags_path[] = "/etc/lisa/tags";

static mm_ptr_t __cfg_get_if_tag(char *if_name) {
	mm_ptr_t ret;

	mm_list_for_each(cfg, ret, mm_ptr(cfg, &CFG->if_tags)) {
		struct if_tag *tag =
			mm_addr(cfg, mm_list_entry(ret, struct if_tag, lh));
		if (!strcmp(if_name, tag->if_name))
			return ret;
	}

	return MM_NULL;
}

int cfg_get_if_tag(char *if_name, char *tag) {
	mm_ptr_t ptr;
	struct if_tag *s_tag;

	cfg_lock();
	ptr = __cfg_get_if_tag(if_name);
	if (MM_NULL == ptr) {
		cfg_unlock();
		return 1;
	}
	s_tag = mm_addr(cfg, mm_list_entry(ptr, struct if_tag, lh));
	strcpy(tag, s_tag->tag);

	cfg_unlock();
	return 0;
}

static int __cfg_del_if_tag(char *if_name) {
	mm_ptr_t lh = __cfg_get_if_tag(if_name);

	if (MM_NULL == lh)
		return 1;
	mm_list_del(cfg, lh);
	mm_free(cfg, mm_list_entry(lh, struct if_tag, lh));
	return 0;
}

static mm_ptr_t __cfg_get_tag_if(char *tag) {
	mm_ptr_t ret;

	mm_list_for_each(cfg, ret, mm_ptr(cfg, &CFG->if_tags)) {
		struct if_tag *s_tag =
			mm_addr(cfg, mm_list_entry(ret, struct if_tag, lh));
		if (!strcmp(tag, s_tag->tag))
			return ret;
	}

	return MM_NULL;
}

int cfg_set_if_tag(char *if_name, char *tag, char *other_if) {
	mm_ptr_t lh, mm_s_tag;
	struct if_tag *s_tag;
	int ret;

	cfg_lock();

	if (NULL == tag) {
		ret = __cfg_del_if_tag(if_name);
		cfg_unlock();
		return ret;
	}

	lh = __cfg_get_tag_if(tag);
	if (MM_NULL != lh) {
		if (NULL != other_if) {
			s_tag = mm_addr(cfg, mm_list_entry(lh, struct if_tag, lh));
			strcpy(other_if, s_tag->if_name);
		}
		cfg_unlock();
		return 1;
	}

	lh = __cfg_get_if_tag(if_name);
	if (MM_NULL != lh) {
		s_tag = mm_addr(cfg, mm_list_entry(lh, struct if_tag, lh));
		strncpy(s_tag->tag, tag, CLI_MAX_TAG);
		s_tag->tag[CLI_MAX_TAG] = '\0';
		cfg_unlock();
		return 0;
	}

	mm_s_tag = mm_alloc(cfg, sizeof(struct if_tag));
	/* first save mm pointer obtained from mm_alloc, then compute s_tag
	 * pointer, because mm_alloc() can change cfg->area if the shm area
	 * is extended (refer to README.mm for details) */
	s_tag = mm_addr(cfg, mm_s_tag);
	if (NULL == s_tag) {
		if (NULL != other_if)
			*other_if = '\0';
		cfg_unlock();
		return 1;
	}

	strncpy(s_tag->if_name, if_name, IFNAMSIZ);
	strncpy(s_tag->tag, tag, CLI_MAX_TAG);
	s_tag->tag[CLI_MAX_TAG] = '\0';
	mm_list_add(cfg, mm_ptr(cfg, &s_tag->lh), mm_ptr(cfg, &CFG->if_tags));

	cfg_unlock();
	return 0;
}

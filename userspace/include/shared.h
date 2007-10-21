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

#ifndef _SHARED_H
#define _SHARED_H

#include "mm.h"

#define CLI_PASS_LEN 30
#define CLI_SECRET_LEN 30
#define CLI_MAX_ENABLE 15
#define CLI_MAX_VTY 15

#define CLI_MAX_TAG 40

struct cli_vty_config {
	char passwd[CLI_PASS_LEN + 1];
};

struct cli_config {
	char enable_secret[CLI_MAX_ENABLE + 1][CLI_SECRET_LEN + 1];
	struct cli_vty_config vty[CLI_MAX_VTY + 1];
};

extern struct mm_private *cfg;
#define CFG ((struct cli_config *)MM_STATIC(cfg))

extern int cfg_init(void);

static __inline__ void cfg_lock(void)
{
	mm_lock(cfg);
}

static __inline__ void cfg_unlock(void) {
	mm_unlock(cfg);
}

extern int cfg_checkpass(int, int (*)(char *, void *), void *);
extern int cfg_waitcr(void);
extern int read_key(void);

extern const char config_file[];
extern const char config_tags_path[];

/* lookup interface arg0 and put tag into arg1; return 0 if
 * interface has a tag, 1 otherwise
 */
extern int cfg_get_if_tag(char *, char *);

/* 1. if arg1 is not null, then assign tag arg1 to interface
 * arg0; if arg2 is not NULL and tag is already assigned to
 * another interface, put the other interface's name into arg2.
 * return 0 if operation successful, 1 if tag is already assigned
 * to another interface
 * 2. if arg1 is null, delete tag for interface arg0. return 0 if
 * successfull, 1 if interface had no tag assigned.
 */
extern int cfg_set_if_tag(char *, char *, char *);
#endif

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

#include "cdp_client.h"

#define SW_CONFIG_FILE	"/etc/lisa/config.text";
#define SW_TAGS_FILE	"/etc/lisa/tags";

#define SW_MAX_VTY		15
#define SW_MAX_ENABLE	15

#define SW_PASS_LEN 	30
#define SW_SECRET_LEN 	30

#define SW_MAX_TAG		40

/* Identifiers for the types of passwords stored in the
 * shared memory area
 */
enum {
	SHARED_AUTH_VTY,
	SHARED_AUTH_ENABLE,
};

/* Initialize the switch's shared memory area */
int shared_init(void);

/* The caller invokes this function requesting the type of
 * authentication and level. In turn, the function will call
 * the user's auth() callback function with the appropriate
 * password from the shared memory area, passing it back the
 * private data pointer.
 */
int shared_auth(int type, int level,
		int (*auth)(char *pw, void *priv), void *priv);

/* Stores the requested password in the shared memory area */
int shared_set_passwd(int type, int level, char *passwd);

/* lookup interface arg0 and put tag into arg1; return 0 if
 * interface has a tag, 1 otherwise
 */
int shared_get_if_tag(int if_index, char *tag);

/* 1. if arg1 is not null, then assign tag arg1 to interface
 * arg0; if arg2 is not NULL and tag is already assigned to
 * another interface, put the other interface's name into arg2.
 * return 0 if operation successful, 1 if tag is already assigned
 * to another interface
 * 2. if arg1 is null, delete tag for interface arg0. return 0 if
 * successfull, 1 if interface had no tag assigned.
 */
int shared_set_if_tag(int if_index, char *tag, int *other_if);

/* Sets the cdp global configuration */
void shared_set_cdp(struct cdp_configuration *cdp);

/* Gets the cdp global configuration */
void shared_get_cdp(struct cdp_configuration *cdp);

#endif

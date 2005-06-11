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

#define CLI_PASS_LEN 30
#define CLI_SECRET_LEN 30
#define CLI_MAX_ENABLE 15
#define CLI_MAX_VTY 15

struct cli_vty_config {
	char passwd[CLI_PASS_LEN + 1];
};

struct cli_config {
	char enable_secret[CLI_MAX_ENABLE + 1][CLI_SECRET_LEN + 1];
	struct cli_vty_config vty[CLI_MAX_VTY + 1];
};

extern struct cli_config *cfg;

extern int cfg_init(void);
extern int cfg_lock(void);
extern int cfg_unlock(void);
extern int cfg_checkpass(int, int (*)(char *, void *), void *);
extern int cfg_waitcr(void);
extern int read_key(void);

#endif

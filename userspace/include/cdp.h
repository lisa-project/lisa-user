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

#ifndef __CDP_H
#define __CDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cdp_ipc.h"

extern int cdp_adm_query(int, char *);

extern void cmd_sh_cdp(FILE *, char **);
extern void cmd_sh_cdp_int(FILE *, char **);
extern void cmd_sh_cdp_ne(FILE *, char **);
extern void cmd_sh_cdp_ne_int(FILE *, char **);
extern void cmd_sh_cdp_ne_detail(FILE *, char **);
extern void cmd_sh_cdp_entry(FILE *, char **);
extern void cmd_sh_cdp_holdtime(FILE *, char **);
extern void cmd_sh_cdp_run(FILE *, char **);
extern void cmd_sh_cdp_timer(FILE *, char **);
extern void cmd_sh_cdp_traffic(FILE *, char **);
extern void cmd_cdp_version(FILE *, char **);
extern void cmd_cdp_run(FILE *, char **);		/* enable cdp globally */
extern void cmd_cdp_holdtime(FILE *, char **);
extern void cmd_cdp_timer(FILE *, char **);
extern void cmd_no_cdp_v2(FILE *, char **);
extern void cmd_no_cdp_run(FILE *, char **);	/* disable cdp globally */
extern void cmd_cdp_if_enable(FILE *, char **);
extern void cmd_cdp_if_disable(FILE *, char **);

#endif

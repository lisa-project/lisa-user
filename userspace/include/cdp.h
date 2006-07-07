#ifndef __CDP_H
#define __CDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
extern void cmd_sh_cdp_interface(FILE *, char **);
extern void cmd_cdp_version(FILE *, char **);
extern void cmd_cdp_run(FILE *, char **);		/* enable cdp globally */
extern void cmd_cdp_holdtime(FILE *, char **);
extern void cmd_cdp_timer(FILE *, char **);
extern void cmd_no_cdp_v2(FILE *, char **);
extern void cmd_no_cdp_run(FILE *, char **);	/* disable cdp globally */
extern void cmd_cdp_if_enable(FILE *, char **);
extern void cmd_cdp_if_disable(FILE *, char **);

#endif

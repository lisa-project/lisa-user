#ifndef _SWCLI_COMMON_H
#define _SWCLI_COMMON_H

#include "rlshell.h"

char *swcli_prompt(struct rlshell_context *ctx);

#define PRIV(x) (1 << (x))
#define PRIV_FILTER(x) ((1 << ((x) + 1)) - 1)

#define MENU_NAME_MAX 32

#endif

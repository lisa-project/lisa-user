#ifndef _SWCLI_COMMON_H
#define _SWCLI_COMMON_H

#include "rlshell.h"

char *swcli_prompt(struct rlshell_context *ctx);

#define PRIV(x) (1 << (x))
#define PRIV_FILTER(x) ((1 << ((x) + 1)) - 1)

#define MENU_NAME_MAX 32


// FIXME move these to an appropriate place
int swcli_tokenize_line(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_number(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);

enum {
	VALID_LIMITS,
	VALID_LIST
};

#endif

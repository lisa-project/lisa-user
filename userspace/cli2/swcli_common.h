#ifndef _SWCLI_COMMON_H
#define _SWCLI_COMMON_H

#include "rlshell.h"

char *swcli_prompt(struct rlshell_context *ctx);

#define PRIV(x) ((uint32_t)1 << (x))
#define VA_PRIV(NIL, priv, ...) PRIV(priv)
#define PRIV_FILTER(x) (((uint32_t)1 << ((x) + 1)) - 1)
#define PRIV_MAX 15
#define PRIV_SHIFT (PRIV_MAX + 1)

#define MENU_NAME_MAX 32

struct swcli_context {
	int ifindex;
	int vlan;
};

// FIXME move these to an appropriate place
int swcli_tokenize_line(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_number(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word_mixed(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);

enum {
	VALID_LIMITS,
	VALID_LIST
};

#define EX_STATUS_REASON_IOCTL(__ctx, __errno) \
	EX_STATUS_REASON(__ctx, "ioctl() failed (%d - %s)", __errno, strerror(__errno))

int cmd_ioctl_simple(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev);

#endif

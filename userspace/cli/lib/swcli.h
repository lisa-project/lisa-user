#ifndef _SWCLI_H
#define _SWCLI_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>

#include "swsock.h"
#include "util.h"
#include "cli.h"
#include "cdp_client.h"
#include "rlshell.h"
#include "interface.h"

char *swcli_prompt(struct rlshell_context *ctx);

#define PRIV(x) ((uint32_t)1 << (x))
#define VA_PRIV(NIL, priv, ...) PRIV(priv)
#define PRIV_FILTER(x) (((uint32_t)1 << ((x) + 1)) - 1)
#define PRIV_MAX 15
#define PRIV_SHIFT (PRIV_MAX + 1)

#define MENU_NAME_MAX 32

struct swcli_context {
	struct rlshell_context rlc;
	int ifindex;
	int vlan;
	int sock_fd;
	struct cdp_session cdp;
};

#define SWCLI_CTX(ctx) ((struct swcli_context *)(ctx))

#define SW_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (SWCLI_CTX(__ctx)->sock_fd == -1) {\
		__sock_fd =  socket(PF_SWITCH, SOCK_RAW, 0); \
		assert(sock_fd != -1);\
	} else\
		__sock_fd = SWCLI_CTX(__ctx)->sock_fd;\
} while (0)

#define SW_SOCK_CLOSE(__ctx, __sock_fd) do {\
	if (__sock_fd != SWCLI_CTX(__ctx)->sock_fd)\
		close(sock_fd);\
} while (0)

// FIXME move these to an appropriate place (suggestion: userspace/cli/lib/cli.h?)
int swcli_tokenize_line(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_number(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_mac(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word_mixed(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_line_mixed(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);

// FIXME move this to appropriate place
#define default_vlan_name(__lvalue, __vlan) do {\
	int status; \
	__lvalue = alloca(9); \
	status = snprintf(__lvalue, 9, "VLAN%04d", (__vlan)); \
	assert(status < 9); \
} while (0)

enum {
	VALID_LIMITS,
	VALID_LIST
};

#define EX_STATUS_REASON_IOCTL(__ctx, __errno) \
	EX_STATUS_REASON(__ctx, "ioctl() failed (%d - %s)", __errno, strerror(__errno))

static __inline__ int __shift_arg(int *argc, char ***argv, struct menu_node ***nodev, int shift) {
	*argv += shift;
	*nodev += shift;
	return *argc -= shift;
}

#define __SHIFT_ARG(__argc, __argv, __nodev, __shift, ...) \
	__shift_arg(&(__argc), &(__argv), &(__nodev), __shift)
#define SHIFT_ARG(__argc, __argv, __nodev, __shift...) \
	__SHIFT_ARG(__argc, __argv, __nodev, ##__shift, 1)

int cmd_ioctl_simple(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev);

#endif

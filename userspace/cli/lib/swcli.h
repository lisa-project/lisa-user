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
#include <linux/sockios.h>

#ifdef LiSA
	#include "swsock.h"
	#include "linux/net_switch.h"
#endif

#include "util.h"
#include "switch_api/sw_api.h"
#include "switch.h"
#include "cli.h"
#include "cdp_client.h"
#include "rstp_client.h"
#include "rlshell.h"
#include "interface.h"
#include "tokenizers.h"

char *swcli_prompt(struct rlshell_context *ctx);
int swcli_dump_args(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev);

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
	struct cdp_session *cdp;
	struct rstp_session *rstp;
};

#define SWCLI_CTX(ctx) ((struct swcli_context *)(ctx))

#define SW_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (SWCLI_CTX(__ctx)->sock_fd != -1) {\
		__sock_fd = SWCLI_CTX(__ctx)->sock_fd;\
		break;\
	}\
	__sock_fd = socket(AF_INET, SOCK_DGRAM, 0); \
	if (__sock_fd == -1) {\
		EX_STATUS_REASON(__ctx, "%s", strerror(errno)); \
		return CLI_EX_REJECTED; \
	}\
} while(0)

#define SW_SOCK_CLOSE(__ctx, __sock_fd) do {\
	if (__sock_fd != SWCLI_CTX(__ctx)->sock_fd)\
		close(__sock_fd);\
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

static __inline__ void init_mac_filter(int *ifindex, int *vlan, int *mac_type,
		unsigned char *mac)
{
	(*ifindex) = 0;
	memset(mac, 0x0, ETH_ALEN);
	(*mac_type) = SW_FDB_ANY;
	(*vlan) = 0;
}

#endif

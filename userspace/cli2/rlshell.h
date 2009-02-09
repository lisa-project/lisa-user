#ifndef _RLSHELL_H
#define _RLSHELL_H

#include "cli.h"

struct rlshell_context {
	/* Need to pass this to underlaying CLI Parser. This must be the
	 * first member,because upper layers must be able to directly cast
	 * a "struct cli_context *" into a "struct rlshell_context *".
	 *
	 * Explanation: since the CLI Parser is not readline-aware, it
	 * cannot pass "struct rlshell_context *" to command handlers. But
	 * command handlers are implemented in upper layers, which are
	 * readline-aware and therefore might need to access the whole
	 * rlshell_context structure.
	 */
	struct cli_context cc;

	/* Callback to build prompt string. Returned buffer must be
	 * alloc'ed malloc()-style and is automatically free()'ed by the
	 * rlshell functions.
	 */
	char *(*prompt)(struct rlshell_context *ctx);

	/* Exit condition from the shell main loop. Must be initialized to
	 * 0 and set to 1 to break the shell loop.
	 */
	int exit;

	/* Prompt length; used to correctly compute the offset to display
	 * the '^' marker on invalid commands. */
	int plen;

	/* Specifies whether rlshell is doing completion, inline help
	 * or exec. Special tokenizers may use this as a hint to
	 * suppress certain matches (and therefore prevent completion),
	 * for instance with special nodes like LINE or WORD */
	int state;

	int enable_ctrl_z;

	/* Upper layer context data */
	void *uc;

	/* Pre-opened PF_SWITCH socket to be used for batch processing
	 * (i.e. swcfgload) */
	int sock_fd;
};

#define SW_SOCK_OPEN(__ctx, __sock_fd) do {\
	if (((struct rlshell_context *)(__ctx))->sock_fd == -1) {\
		__sock_fd =  socket(PF_SWITCH, SOCK_RAW, 0); \
		assert(sock_fd != -1);\
	} else\
		__sock_fd = ((struct rlshell_context *)(__ctx))->sock_fd;\
} while (0)

#define SW_SOCK_CLOSE(__ctx, __sock_fd) do {\
	if (__sock_fd != ((struct rlshell_context *)(__ctx))->sock_fd)\
		close(sock_fd);\
} while (0)

/**
 * Possible values for state in struct rlshell_context
 */
enum {
	RLSHELL_COMPLETION,
	RLSHELL_HELP,
	RLSHELL_EXEC
};

int rlshell_main(struct rlshell_context *ctx);
struct rlshell_context *rlshell_get_context(void);

#endif

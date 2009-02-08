#ifndef _CLI_H
#define _CLI_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

struct menu_node;

#define MAX_MENU_DEPTH 64
#define TOKENIZE_MAX_MATCHES 128

#ifndef whitespace
#define whitespace(c) ((c) == ' ' || (c) == '\t')
#endif

/* Tokenizer output */
struct tokenize_out {
	/* Offset of the current token in the input buffer */
	int offset;
	/* Length of the current token */
	int len;
	/* Length of the initial token substring that would produce at least
	 * one match */
	int ok_len;
	/* Array of matching menu nodes */
	struct menu_node *matches[TOKENIZE_MAX_MATCHES+1];
	/* Exact match (token length == node name length) if any */
	struct menu_node *exact_match;
	/* Partially matching node (when no matches found), if any */
	struct menu_node *partial_match;
};

#define MATCHES(out) ((out)->matches[0] == NULL ? 0 : ((out)->matches[1] == NULL ? 1 : 2))

/* CLI state context information */
struct cli_context {
	/* Bitwise selector against menu node masks (nodes that don't match
	 * are filtered)
	 */
	uint32_t node_filter;

	/* Additional hints about last command execution failure */
	union {
		/* Parse error offset for INVALID commands */
		int offset;

		/* Error description for REJECTED commands */
		char *reason;
	} ex_status;

	/* Function to be called by command handler to get the output stream */
	FILE *(*out_open)(struct cli_context *ctx, int paged);

	/* Output pager context */
	struct {
		/* Pid of the forked pager process */
		pid_t pid;
		/* Pipe: the command handler or filter write to p[1] and
		 * the pager reads from p[0]
		 */
		int p[2];
	} pager;

	/* Output filter context */
	struct {
		/* Additional information is stored here by the filter command handler
		 * FIXME: @blade: any good reason for this to be void*? Btw, why even
		 * pointer to another type of structure and not embed those members here?
		 */
		void *priv;

		/* Launches the filter program:
		 * executes the necessary pipe()/fork()/close()/dup2()/exec() sequence of
		 * operations, ensuring that the output of the filter program is sent to
		 * out_fd
		 */
		int (*open)(struct cli_context *ctx, int out_fd);

		/* Closes the remaing open fds and waitpid() for the filter program */
		int (*close)(struct cli_context *ctx);
	} filter;

	/* Current menu node root */
	struct menu_node *root;
};

static inline void cli_init_tok_out(struct tokenize_out *out)
{
	out->ok_len = 0;
	out->exact_match = NULL;
	out->partial_match = NULL;
	out->len = 0;
}

#define EX_STATUS_REASON(ctx, fmt, par...) do {\
	if (asprintf(&(ctx)->ex_status.reason, fmt, ##par) == -1)\
		(ctx)->ex_status.reason = NULL;\
} while (0)

/* Command tree menu node */
struct menu_node {
	/* Complete name of the menu node */
	const char *name;

	/* Help message */
	const char *help;

	/* Bitwise mask for filtering */
	uint32_t *mask;

	/* Custom tokenize function for the node */
	int (*tokenize)(struct cli_context *ctx, const char *buf,
					struct menu_node **tree, struct tokenize_out *out);

	/* Command handler for runnable nodes; ctx is a passed-back pointer,
	 * initially sent as argument to cli_exec(); argc is the number of
	 * tokens in the command; tokv is the array of tokens (exactly as
	 * they appear in the input command); nodev is the array of matching
	 * menu nodes, starting from root.
	 */
	int (*run)(struct cli_context *ctx, int argc, char **tokv,
					struct menu_node **nodev);

	/* Additional data that a custom tokenize function may use */
	void *priv;

	/* Points to the sub menu of the node */
	struct menu_node **subtree;
};

#define CLI_MASK(...) &(uint32_t []){__VA_ARGS__, 0}[0]

static inline int cli_mask_apply(uint32_t *mask, uint32_t filter) {
	if (!mask)
		return 1;
	while (*mask) {
		if (!(*mask & filter))
			return 0;
		mask++;
	}
	return 1;
}

/* CLI output filter private data fields */
struct cli_filter_priv {
	/* Pid of the forked filter process */
	pid_t pid;
	/* Pipe: command handler writes to p[1] and the filter
	 * program reads from p[0].
	 */
	int p[2];

	/* Arguments: the filter handler stores here all the
	 * command line arguments passed after the filter
	 * separator: for example in swcli, all the command
	 * line arguments following the '|' token.
	 */
	const char **argv;
};

/* CLI command execution component return codes.
 */
enum {
	/* Codes returned by parser */
	CLI_EX_AMBIGUOUS		= 1,
	CLI_EX_INCOMPLETE		= 2,
	CLI_EX_INVALID			= 3,

	/* Codes returned by handlers */
	CLI_EX_OK				= 0,
	CLI_EX_REJECTED			= 4,
	CLI_EX_NOTIMPL			= 5
};

/* CLI token parsing function.
 * Returns the details (offset, length) about the next token in buf through
 * the tokenize_out structure
 */
int cli_next_token(const char *buf, struct tokenize_out *out);

/* CLI tokenizer function.
 * Must be initially called with the whole command as input and the root menu
 * tree node as input. As it extracts the first token from the input, it must
 * be itreratively called on the remaining input buffer. For each extracted
 * token, the context (subnodes list) descends in the menu tree
 */
int cli_tokenize(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

/* CLI execution function.
 * Resposible for invoking the tokenizer function, interpreting its result
 * and act accordingly. In the case of a valid command executes the command
 * handler. In case of error returns the error details through the CLI context
 */
int cli_exec(struct cli_context *ctx, char *cmd);

/* CLI output provider function.
 * Called by the CLI command handler functions in order to obtain a file to
 * write their output to. When the handler function has to do a lot of output
 * it will request the output to be paged. The paging will be automatically
 * handled by the CLI.
 * In case the command was invoked indirectly from an output modifier, or
 * filter handler, the command output will be first passed through the
 * appropriate filter before reaching the pager or stdout.
 */
FILE *cli_out_open(struct cli_context *ctx, int paged);

/* CLI output filter open.
 * Invokes the filter program based on the information passed in the cli
 * context. The output of the filter program will be directed to out_fd.
 */
int cli_filter_open(struct cli_context *ctx, int out_fd);

/* CLI output filter close.
 * Finishes the filter program based on the information provided in the cli
 * context.
 */
int cli_filter_close(struct cli_context *ctx);

#endif

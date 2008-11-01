#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "swcli_common.h"

char *swcli_prompt(struct rlshell_context *ctx) {
	char hostname[HOST_NAME_MAX + 1];
	size_t buf_size = HOST_NAME_MAX + MENU_NAME_MAX + 3;
	char *buf = malloc(buf_size);
	char prompt = ctx->cc.filter & PRIV(2) ? '#' : '>';

	if (buf == NULL)
		return buf;

	gethostname(hostname, HOST_NAME_MAX);
	hostname[HOST_NAME_MAX] = '\0';

	if (ctx->cc.root->name == NULL)
		snprintf(buf, buf_size, "%s%c", hostname, prompt);
	else
		snprintf(buf, buf_size, "%s(%s)%c", hostname,
				ctx->cc.root->name, prompt);

	return buf;
}

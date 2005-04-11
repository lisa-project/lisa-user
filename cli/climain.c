#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "climain.h"

int climain(void) {
	char hostname[MAX_HOSTNAME];
	char prompt[MAX_HOSTNAME + 32];

	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	sprintf(prompt, "%s>", hostname);
	readline(prompt);
	return 0;
}

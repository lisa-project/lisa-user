#include <stdio.h>
#include <unistd.h>

#include "climain.h"
#include "shared.h"

static int password_valid(char *pw, void *arg) {
	return !strcmp(pw, cfg->vty[0].passwd);
}

int max_attempts = 3;

int main(int argc, char **argv) {
	char hostname[MAX_HOSTNAME];
	
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	printf("\r\n%s line %d", hostname, 1);
	printf("\r\n\r\n\r\nUser Access Verification");
	printf("\r\n\r\n");
	fflush(stdout);

	cfg_init();
	if(cfg_checkpass(max_attempts, password_valid, NULL)) {
		climain();
		return 0;
	}

	printf("%% Bad passwords\r\n");
	return 1;
}

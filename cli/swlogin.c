#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

#include "climain.h"

void sw_redisplay(void) {
	fprintf(rl_outstream, "\rPassword: ");
	fflush(rl_outstream);
}

int password_valid(char *pw) {
	return !strcmp(pw, "letmein");
}

int max_attempts = 3;

int main(int argc, char **argv) {
	char hostname[MAX_HOSTNAME];
	char *pw;
	int i;
	
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	printf("\r\n%s line %d", hostname, 1);
	printf("\r\n\r\n\r\nUser Access Verification");
	printf("\r\n\r\n");
	fflush(stdout);

	rl_redisplay_function = sw_redisplay;
	for(i = 0; i < max_attempts; i++) {
		pw = readline(NULL);
		if(password_valid(pw))
			break;
	}
	rl_redisplay_function = rl_redisplay;
	if(i >= max_attempts) {
		printf("%% Bad passwords\r\n");
		return 1;
	}

	climain();

	return 0;
}

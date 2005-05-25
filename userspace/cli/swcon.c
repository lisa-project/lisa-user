#include <stdio.h>
#include <unistd.h>

#include "climain.h"

int main(int argc, char **argv) {
	char hostname[MAX_HOSTNAME];
	
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	printf(
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"
			"\r\n\r\n%s con0 is now available\r\n\r\n\r\n\r\n\r\n\r\n"
			"Press RETURN to get started."
			"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n",
			hostname);
	getchar();
	climain();
	return 0;
}

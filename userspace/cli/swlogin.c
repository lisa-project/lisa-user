/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

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

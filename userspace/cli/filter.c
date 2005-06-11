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

/* filter.c: Output filtering functions */
#include "filter.h"

void begin_filter(regex_t *reg) {
	regmatch_t result;
	char line[MAX_LINE_WIDTH];
	char *ptr;

	while (NULL != (ptr = fgets(line, MAX_LINE_WIDTH, stdin)))
		if (0 == regexec(reg, line, 1, &result, 0)) 
			break;
	if (ptr) {
		printf("%s", ptr);
		while (NULL != fgets(line, MAX_LINE_WIDTH, stdin))
			printf("%s", line);
	}
}

void pass_filter(int mode, regex_t *reg) {
	regmatch_t result;
	char line[MAX_LINE_WIDTH];

	while (NULL != fgets(line, MAX_LINE_WIDTH, stdin))
		if ((mode != MODE_INCLUDE) != !regexec(reg, line, 1, &result, 0))
			printf("%s", line);
}

void grep_filter(char *grep_args[]) {
	int i;
	char **new_args;

	new_args = (char **)malloc(sizeof(char *) * (sizeof(grep_args)+1));
	new_args[0] = strdup(GREP_PATH);
	for (i=0; grep_args[i]; i++)
		new_args[i+1] = grep_args[i];
	new_args[i+1] = NULL;
	execv(GREP_PATH, new_args);
	free(new_args);
}

void filter_stream(char *argv[]) {
	int mode = atoi(argv[1]);
	regex_t regex;
	

	switch (mode) {
	case MODE_BEGIN:
	case MODE_INCLUDE:
	case MODE_EXCLUDE:
		if (regcomp(&regex, argv[2], 0)) {
			perror("regcomp");
			return;
		}
		if (MODE_BEGIN == mode) {
			begin_filter(&regex);	
			break;
		}
		pass_filter(mode, &regex);		
		break;
	case MODE_GREP:
		grep_filter(&argv[2]);
		break;
	}
}

/*
	Usage: ./filter mode pattern|grep_args 
	Example: ./filter 3 /bin/grep -v test 
 */
int main(int argc, char *argv[]) {
	if (argc >= 3)
		filter_stream(argv);
	return 0;
}

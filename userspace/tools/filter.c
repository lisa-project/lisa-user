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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define MAX_LINE_WIDTH 1024
#define GREP_PATH "/bin/grep"

/* Reads line by line from stdin until it finds a
 * line matching the specified regular expression.
 * After a match, all remaining lines are printed
 * to stdout.
 */
static int begin(regex_t *reg)
{
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
	return 0;
}

/* Reads line by line from stdin.
 * If exclude is equal to 0, all lines matching the regular
 * expression are printed to stdout. Otherwise all lines that
 * don't match the regular expression are printed to stdout.
 */
static int pass(regex_t *reg, int exclude)
{
	regmatch_t result;
	char line[MAX_LINE_WIDTH];

	while (NULL != fgets(line, MAX_LINE_WIDTH, stdin))
		if (exclude != !regexec(reg, line, 1, &result, 0))
			printf("%s", line);
	return 0;
}

/* Executes grep with the given arguments.  */
static int grep(char *grep_args[])
{
	int i;
	char **new_args;

	new_args = (char **)malloc(sizeof(char *) * (sizeof(grep_args)+1));
	new_args[0] = strdup(GREP_PATH);
	for (i=0; grep_args[i]; i++)
		new_args[i+1] = grep_args[i];
	new_args[i+1] = NULL;
	execv(GREP_PATH, new_args);
	free(new_args);

	return 0;
}

/* Program help message */
static int usage(const char *progname)
{
	printf( "Usage: %s <type> <args>, where type can be one of: \n"
			"\tinclude - include filter\n"
			"\texclude - exclude filter\n"
			"\tbegin - begin filter\n"
			"\tgrep - grep filter\n",
			progname);
	return 1;
}

int main(int argc, char *argv[]) {
	const char *type;
	regex_t regex;

	if (argc < 3) {
		printf("Error: invalid number of arguments: %d\n", argc);
		return usage(argv[0]);
	}

	type = argv[1];
	if (!strcmp(type, "grep"))
		return grep(&argv[2]);

	if (regcomp(&regex, argv[2], 0)) {
		perror("regcomp");
		return usage(argv[0]);
	}

	if (!strcmp(type, "begin"))
		return begin(&regex);	

	if (strcmp(type, "include") && strcmp(type, "exclude")) {
		printf("Error: invalid filter type '%s'\n", type);
		return usage(argv[0]);
	}

	return pass(&regex, !strcmp(type, "exclude"));
}

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

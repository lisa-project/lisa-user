#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>

#include "command.h"

char buf[1024 * 1024];
char *bufp = buf;

char *hdlrs[128];
int hdlr = 0;

char *argv0;

void hdlr_add(char *sym) {
	int i;

	if (sym == NULL)
		return;

	for (i = 0; i < hdlr; i++)
		if (!strcmp(hdlrs[i], sym))
			return;

	hdlrs[hdlr++] = strdup(sym);
}

char *guess_ptr(void *p, const char *self) {
	char buf[1024];
	int pipefd[2];
	FILE *f;
	char *ret = NULL;
	pid_t cpid;
	int status;

	pipe(pipefd);

	if (!(cpid = fork())) {
		const char * argv[] = {
			"/usr/bin/objdump",
			"-t",
			self,
			NULL
		};

		dup2(pipefd[1], 1);
		execv(argv[0], (char * const *)argv);
	}

	close(pipefd[1]);

	if (cpid == -1) {
		close(pipefd[0]);
		return NULL;
	}

	f = fdopen(pipefd[0], "r");
	while (fgets(buf, sizeof(buf), f)) {
		void *lp;
		char ln[256];

		if (ret != NULL)
			continue;

		// 08048645 g     F .text      00000050              main
		if (sscanf(buf, "%p%*c%*c%*c%*c%*c%*c%*c%*c %*s %*s %s", &lp, ln) < 2)
			continue;
		
		if (lp == p)
			ret = strdup(ln);
	}
	fclose(f);
	waitpid(cpid, &status, 0);

	return ret;
}

void rec(sw_command_t *cmd, int indent, const char *cmdline) {
	char pad[64];
	char path[128];
	// int sep = 0;

	memset(pad, '\t', sizeof(pad));
	pad[indent + 1] = '\0';


	while (cmd->name) {
		char *run = NULL;

		if (cmd->func)
			run = guess_ptr(cmd->func, argv0);

		hdlr_add(run);

		sprintf(path, "%s%s ", cmdline, cmd->name);

		/*
		if (sep)
			bufp += sprintf(bufp, ",\n\n");
		sep = 1;
		*/

		pad[indent] = '\0';
		bufp += sprintf(bufp, "%s/* %s*/\n", pad, path);
		bufp += sprintf(bufp, "%s& (struct menu_node){\n", pad);

		pad[indent] = '\t';
		bufp += sprintf(bufp,
				"%s.name\t\t\t= \"%s\",\n"
				"%s.help\t\t\t= \"%s\",\n"
				"%s.mask\t\t\t= PRIV(%d),\n"
				"%s.tokenize\t= NULL,\n"
				"%s.run\t\t\t= %s,\n"
				"%s.subtree\t= ",

				pad, cmd->name,
				pad, cmd->doc,
				pad, cmd->priv,
				pad,
				pad, run ? run : "NULL",
				pad
				);

		if (cmd->subcmd) {
			bufp += sprintf(bufp, "(struct menu_node *[]) { /*{{{*/\n");
			rec(cmd->subcmd, indent + 2, path);
			bufp += sprintf(bufp, "\n%s} /*}}}*/\n", pad);
		} else {
			bufp += sprintf(bufp, "NULL\n");
		}

		pad[indent] = '\0';
		// bufp += sprintf(bufp, "%s}", pad);
		bufp += sprintf(bufp, "%s},\n\n", pad);

		if (run != NULL)
			free(run);

		cmd++;
	}

	bufp += sprintf(bufp, "%sNULL", pad);
}

int main(int argc, char **argv) {
	int i;

	argv0 = argv[0];

	printf(
			"#include \"cli.h\"\n"
			"#include \"swcli_common.h\"\n"
			"\n"
		  );

	rec(command_root_main.cmd, 2, "#");

	for (i = 0; i < hdlr; i++)
		printf("int %s(struct cli_context *, int, char **, struct menu_node **);\n",
				hdlrs[i]);

	printf(
			"\nstruct menu_node menu_main = {\n"
			"\t/* Root node, .name is used as prompt */\n"
			"\t.name\t\t\t= NULL,\n"
			"\t.subtree\t= (struct menu_node *[]) {\n"
			"%s\n"
			"\t}\n"
			"};\n"
			"\n"
			"/* vim: ts=2 shiftwidth=2 foldmethod=marker\n"
			"*/\n",

			buf
		  );
	return 0;
}

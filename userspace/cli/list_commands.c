#include <sys/types.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#include "swcli.h"

#define MAX_DEPTH 32

extern struct menu_node menu_main;
extern struct menu_node config_main;
extern struct menu_node config_if_main;
extern struct menu_node config_vlan_main;
extern struct menu_node config_line_main;

char *argv0;

const char *stack[MAX_DEPTH];

//what = 1 ==> find symbol name by pointer
//what = 0 ==> find pointer by symbol name
void *guess_ptr(void *p, int what) {
	char buf[1024];
	int pipefd[2];
	FILE *f;
	void *ret = NULL;
	pid_t cpid;
	int status;

	pipe(pipefd);

	if (!(cpid = fork())) {
		const char * argv[] = {
			"/usr/bin/objdump",
			"-t",
			"libswcli.so",
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
		// 000075f0 g     F .text      00000149              rlshell_main
		if (sscanf(buf, "%p%*c%*c%*c%*c%*c%*c%*c%*c %*s %*s %s", &lp, ln) < 2)
			continue;
		
		if(!what){
			if (lp == p)
				ret = strdup(ln);
		}else
			if (!strcmp(ln, p))
				ret = lp;
	}
	fclose(f);
	waitpid(cpid, &status, 0);
	
	return ret;
}

void dump_cmd(struct menu_node *node, int lev, unsigned int p) {
	struct menu_node **np;
	int i;

	assert(lev < MAX_DEPTH);
	stack[lev] = node->name;

	if (!lev)
		printf(node->name ? "(%s)#\n" : "#\n", node->name);

	if (node->run) {
		for (i = 1; i <= lev; i++)
			printf("%s ", stack[i]);
		if(p>0) 
			printf("[%s]",(char *)guess_ptr((void*)((char*)node->run-p),0));
		printf("\n");
	}

	if (node->subtree == NULL)
		return;
	
	for (np = node->subtree; *np; np++) {
		if (!strcmp((*np)->name, "|"))
			continue;
		dump_cmd(*np, lev + 1, p);
	}

	if (!lev)
		printf("\n");
}

int main(int argc, char **argv) {
	
	unsigned int offset = 0;
	int opt;
	char anchor[] = "rlshell_main";
	
	 while ((opt = getopt(argc, argv, "s")) != -1) {
		 switch (opt) {
			 case 's':
				 offset = (char*)rlshell_main-(char*)guess_ptr(anchor,1);
				 //printf("[%s] -> %x",anchor,offset);
			 break;
		 }	
	 }

	dump_cmd(&menu_main, 0, offset);
	dump_cmd(&config_main, 0, offset);
	dump_cmd(&config_if_main, 0, offset);
	dump_cmd(&config_vlan_main, 0, offset);
	dump_cmd(&config_line_main, 0, offset);

	return 0;
}

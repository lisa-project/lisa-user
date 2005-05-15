#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/param.h>
#include <signal.h>
#include <stdlib.h>

#include "command.h"
#include "climain.h"
#include "config.h"
#include "build_config.h"
#include "filter.h"
#include "if.h"

static char if_name[IFNAMSIZ];

static void swcli_sig_term(int sig) {
	char hostname[MAX_HOSTNAME];
	cmd_root = &command_root_main;
	signal(SIGTSTP, SIG_IGN);
	printf("\n");
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	sprintf(prompt, cmd_root->prompt, hostname, priv ? '#' : '>');
	rl_set_prompt(prompt);
	rl_forced_update_display();
}

/* Command Handlers implementation */
static void cmd_disable(FILE *out, char **argv) {
	priv = 1;
}

static void cmd_enable(FILE *out, char **argv) {
	int req;

	req = argv[0] == NULL ? 15 : atoi(argv[0]);
	if(req > priv) {
	}
	priv = 15;
}

static void cmd_conf_t(FILE *out, char **argv) {
	cmd_root = &command_root_config;
	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	signal(SIGTSTP, swcli_sig_term);
}

void cmd_help(FILE *out, char **argv) {
	fprintf(out,
		"Help may be requested at any point in a command by entering\n"
		"a question mark '?'.  If nothing matches, the help list will\n"
		"be empty and you must backup until entering a '?' shows the\n"
		"available options.\n"
		"Two styles of help are provided:\n"
		"1. Full help is available when you are ready to enter a\n"
		"   command argument (e.g. 'show ?') and describes each possible\n"
		"   argument.\n"
		"2. Partial help is provided when an abbreviated argument is entered\n"
		"   and you want to know what arguments match the input\n"
		"   (e.g. 'show pr?'.)\n\n"
		);
}

static void cmd_history(FILE *out, char **argv) {
	HIST_ENTRY **history;
	HIST_ENTRY *entry;
	int i;

	history = history_list();
	if (history) {
		for (i=0; (entry = history[i]); i++) {
			fprintf(out, "   %s\n", entry->line);
		}
	}
}

static void cmd_exit(FILE *out, char **argv) {
	pclose(out);
	exit(0);
}

static void cmd_run_eth(FILE *out, char **argv) {
	char *arg = *argv;
	int status;
	FILE *tmp = NULL;
	char tmp_name[MAXPATHLEN];

	fprintf(out, "Building configuration...\n");
	fflush(out);
	do {
		tmp = mk_tmp_stream(tmp_name, "w+");
		if(tmp == NULL)
			break;
		status = build_int_eth_config(tmp, parse_eth(arg));
		if(status)
			break;
		fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(tmp));
		rewind(tmp);
		copy_data(out, tmp);
		fclose(tmp);
		unlink(tmp_name);
		fprintf(out, "end\n\n");
		fflush(out);
		return;
	} while(0);
	if(tmp != NULL) {
		fclose(tmp);
		unlink(tmp_name);
	}
	fprintf(out, "Command rejected: device not present\n");
	return;
}

static void cmd_show_priv(FILE *out, char **argv) {
	fprintf(out, "Current privilege level is %d\n", priv);
	fflush(out);
}

static void cmd_show_run(FILE *out, char **argv) {
	int status;
	FILE *tmp = NULL;
	char tmp_name[MAXPATHLEN];

	fprintf(out, "Building configuration...\n");
	fflush(out);
	do {
		tmp = mk_tmp_stream(tmp_name, "w+");
		if(tmp == NULL)
			break;
		status = build_config(tmp);
		if(status)
			break;
		fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(tmp));
		rewind(tmp);
		copy_data(out, tmp);
		fclose(tmp);
		unlink(tmp_name);
		fprintf(out, "!\nend\n\n");
		fflush(out);
		return;
	} while(0);
	if(tmp != NULL) {
		fclose(tmp);
		unlink(tmp_name);
	}
	fprintf(out, "Oops! Something went terribly wrong :(\n");
	return;
}

static void cmd_run_vlan(FILE *out, char **argv) {
}

static void init_mac_filter(struct net_switch_ioctl_arg *user_arg) {
	user_arg->if_name = NULL;
	user_arg->cmd = SWCFG_GETMAC;
	memset(&user_arg->ext.marg.addr, 0, ETH_ALEN);
	user_arg->ext.marg.addr_type = SW_FDB_ANY;
	user_arg->vlan = 0;
}

static void do_mac_filter(FILE *out, struct net_switch_ioctl_arg *user_arg, int size, char *buf) {
	int status;

	do {
		user_arg->ext.marg.buf_size = size;
		user_arg->ext.marg.buf = buf;
		status = ioctl(sock_fd, SIOCSWCFG, user_arg);
		if (status == -1) {
			perror("ioctl");
			break;
		}
		if (status == SW_INSUFFICIENT_SPACE) {
			dbg("Insufficient buffer space. Realloc'ing ...\n");
			buf = realloc(buf, size+INITIAL_BUF_SIZE);
			assert(buf);
			size+=INITIAL_BUF_SIZE;
		}
		else {
			user_arg->ext.marg.actual_size = status;
			cmd_showmac(out, (char *)user_arg);
			break;
		}	
	} while(status);
}

static void cmd_show_mac(FILE *out, char **argv) {
	int i, size;
	char *arg, *buf;
	struct net_switch_ioctl_arg user_arg;

	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);
	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
		}
	}
	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_mac_eth(FILE *out, char **argv) {
	char *arg, *buf;
	struct net_switch_ioctl_arg user_arg;
	int i, size;


	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);

	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
			continue;
		}
		if (valid_eth(arg, arg[strlen(arg)-1]) && !user_arg.if_name) {
			user_arg.if_name = if_name_eth(arg);
		}
	}

	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_mac_vlan(FILE *out, char **argv) {
	char *arg, *buf;
	struct net_switch_ioctl_arg user_arg;
	int i, size, a[2], cnt=0, k;

	
	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);
	
	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
			continue;
		}
		if (sscanf(arg, "%d", &k)) {
			a[cnt++] = k;
		}
	}
	if (cnt > 1) { /* si eth si vlan */
		sprintf(if_name, "eth%d", a[0]);
		user_arg.if_name = if_name;
		user_arg.vlan = a[1];
	}
	else user_arg.vlan = a[0];

	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_addr(FILE *out, char **argv) {
	printf("sh_mac_addr\n");
}

/* Validation Handlers Implementation */
int valid_regex(char *arg, char lookahead) {
	return 1;
}

int valid_eth(char *arg, char lookahead) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 0 || no > 7) /* FIXME max value */
		return 0;
	/* FIXME interfata e in switch */
	return 1;
}

int valid_vlan(char *arg, char lookahead) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 1 || no > 4094)
		return 0;
	return 1;
}

int valid_mac(char *arg, char lookahead) {
	regex_t regex;
	regmatch_t result;
	int res;

	if (regcomp(&regex, whitespace(lookahead)? 
				"^[0-9a-f]{1,4}(\\.[0-9a-f]{1,4}){2}$":"^[0-9a-f]{1,4}(\\.[0-9a-f]{0,4}){0,2}$", 
				REG_ICASE|REG_EXTENDED)) {
		perror("regcomp");
		return 0;
	}
	res = regexec(&regex, arg, 1, &result, 0);
	return (res == 0);
}

int valid_dyn(char *arg, char lookahead) {
	return (strcmp(arg, "dynamic") == 0);
}

int valid_static(char *arg, char lookahead) {
	return (strcmp(arg, "static") == 0);
}

char vlan_range[] = "<1-1094>\0";

int parse_eth(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);

	return no;
}

char *if_name_eth(char *arg) {
	sprintf(if_name, "eth%d", parse_eth(arg));
	return if_name;
}

int parse_vlan(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);
	assert(no >= 1 && no <= 4094);
	return no;
}

int parse_mac(char *arg, unsigned char *mac) {
	unsigned short a0, a1, a2;

	if (sscanf(arg, "%hx.%hx.%hx", &a0, &a1, &a2) != ETH_ALEN/2)
		return EINVAL;

	mac[0] = a0 / 0x100;
	mac[1] = a0 % 0x100;
	mac[2] = a1 / 0x100;
	mac[3] = a1 % 0x100;
	mac[4] = a2 / 0x100;
	mac[5] = a2 % 0x100;

	return 0;
}

static int valid_priv(char *arg) {
	int n;

	n = atoi(arg);
	if(n >= 1 && n <= 15)
		return 1;
	return 0;
}

static sw_command_t sh_pipe_regex[] = {
	{"LINE",				1,  valid_regex,	NULL,			RUN,		"Regular Expression",								NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_pipe_mod[] = {
	{"begin",				1,  NULL,			NULL,			MODE_BEGIN,		"Begin with the line that matches",				sh_pipe_regex},
	{"exclude",				1,  NULL,			NULL,			MODE_EXCLUDE,	"Exclude lines that match",						sh_pipe_regex},
	{"include",				1,  NULL,			NULL,			MODE_INCLUDE,	"Include lines that match",						sh_pipe_regex},
	{"grep",				1,  NULL,			NULL,			MODE_GREP,		"Linux grep functionality",						sh_pipe_regex},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

sw_command_t sh_pipe[] = {
	{"|",					1,  NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_int_eth[] = {
	{eth_range,				1,	valid_eth,		cmd_int_eth,	RUN,		"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_int_vlan[] = {
	{vlan_range,			1,	valid_vlan,		cmd_int_vlan,	RUN,		"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_run_eth[] = {
	{eth_range,				2,	valid_eth,		cmd_run_eth,	RUN,		"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_run_vlan[] = {
	{vlan_range,			2,	valid_vlan,		cmd_run_vlan,	RUN,		"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_vlan[] = {
	{vlan_range,			1,	valid_vlan,		cmd_sh_mac_vlan,RUN|PTCNT,	"Vlan interface number",							sh_pipe},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_m_eth[] = {
	{"vlan",				1,	NULL,			NULL,			0,			 "VLAN keyword",									sh_mac_vlan},
	{"|",					1,	NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_eth[] = {
	{eth_range,				0,	valid_eth,		cmd_sh_mac_eth,	RUN|PTCNT,	"Ethernet interface number",						sh_sh_m_eth},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_int[] = {
	{"ethernet",			1,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_int_eth},
	{"vlan",				1,	NULL,			NULL,			0,			"LMS Vlans",										sh_int_vlan},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_run_int[] = {
	{"ethernet",			2,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_run_eth},
	{"vlan",				2,	NULL,			NULL,			0,			"LMS Vlans",										sh_run_vlan},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_mac_int[] = {
	{"ethernet",			1,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_mac_eth},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_mac_addr[] = {
	{"H.H.H",				1,	valid_mac,		cmd_show_mac,	RUN|PTCNT,	"48 bit mac address",								NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_mac_sel[] = {
	{"address",				1,	NULL,			NULL,			0,			"address keyword",									sh_sh_mac_addr},
	{"interface",			1,	NULL,			NULL,			0,			"interface keyword",								sh_sh_mac_int},
	{"vlan",				1,	NULL,			NULL,			0,			 "VLAN keyword",									sh_mac_vlan},
	{"|",					1,	NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_addr_table[] = {
	{"address",				1,	NULL,			NULL,			0,			"address keyword",									sh_sh_mac_addr},
	{"aging-time",			1,	NULL,			NULL,			RUN,		"aging-time keyword",								NULL},
	{"dynamic",				1,	valid_dyn,		cmd_show_mac,	RUN|CMPL|PTCNT,	"dynamic entry type",								sh_sh_mac_sel},
	{"interface",			1,	NULL,			NULL,			0,			"interface keyword",								sh_sh_mac_int},
	{"static",				1,	valid_static,	cmd_show_mac,	RUN|CMPL|PTCNT,	"static entry type",								sh_sh_mac_sel},
	{"vlan",				1,	NULL,			NULL,			0,			 "VLAN keyword",									sh_mac_vlan},
	{"|",					1,	NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_show_run[] = {
	{"interface",			1,	NULL,			NULL,			0,			"Show interface configuration",						sh_sh_run_int},
	{"|",					1,  NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_show[] = {
	{"arp",					1,	NULL,			NULL,			RUN,		"ARP table",										NULL},
	{"clock",				1,	NULL,			NULL,			RUN,		"Display the system clock",							NULL},
	{"history",				1,	NULL,			cmd_history,	RUN,		"Display the session command history",				sh_pipe},
	{"interfaces",			1,	NULL,			cmd_sh_int,		RUN,		"Interface status and configuration",				sh_sh_int},
	{"ip",					1,	NULL,			NULL,			RUN,		"IP information",									NULL},
	{"mac",					1,	NULL,			NULL,			RUN,		"MAC configuration",								NULL},
	{"mac-address-table",	1,	NULL,			cmd_show_mac,	RUN,		"MAC forwarding table",								sh_mac_addr_table},
	{"privilege",			2,	NULL,			cmd_show_priv,	RUN,		"Show current privilege level",						NULL},
	{"running-config",		2,	NULL,			cmd_show_run,	RUN,		"Current operating configuration",					sh_show_run},
	{"sessions",			1,	NULL,			NULL,			RUN,		"Information about Telnet connections",				NULL},
	{"startup-config",		1,	NULL,			NULL,			RUN,		"Contents of startup configuration",				NULL},
	{"users",				1,	NULL,			NULL,			RUN,		"Display information about terminal lines",			NULL},
	{"version",				1,	NULL,			NULL,			RUN,		"System hardware and software status",				NULL},
	{"vlan",				1,	NULL,			NULL,			RUN,		"VTP VLAN status",									NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_conf[] = {
	{"terminal",			2,	NULL,			cmd_conf_t,		RUN,		"Configure from the terminal",						NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_enable[] = {
	{"<1-15>",				1,	valid_priv,		cmd_enable,		RUN|PTCNT,	"Enable level",										NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh[] = {
	{"clear",				1,	NULL, 			NULL,			0,			"Reset functions",									NULL},
	{"configure",			2,	NULL,			NULL,			0,			"Enter configuration mode",							sh_conf},
	{"disable",				2,	NULL,			cmd_disable,	RUN,		"Turn off privileged commands",						NULL},
	{"enable",				1,	NULL,			cmd_enable,		RUN,		"Turn on privileged commands",						sh_enable},
	{"exit",				1,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"help",				1,	NULL,			cmd_help,		RUN,		"Description of the interactive help system",		NULL},
	{"logout",				1,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"ping",				1,	NULL,			NULL,			0,			"Send echo messages",								NULL},
	{"quit",				1,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"show",				1,	NULL,			NULL,			0,			"Show running system information",					sh_show},
	{"telnet",				1,	NULL,			NULL,			0,			"Open a telnet connection",							NULL},
	{"terminal",			1,	NULL,			NULL,			0,			"Set terminal line parameters",						NULL},
	{"traceroute",			1,	NULL,			NULL,			0,			"Trace route to destination",						NULL},
	{"where",				1,	NULL,			NULL,			RUN,		"List active connections",							NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

sw_command_root_t command_root_main =					{"%s%c",					sh};
sw_command_root_t command_root_config_vlan =			{"%s(config-vlan)%c",		NULL};

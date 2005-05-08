#include <linux/net_switch.h>
#include <linux/if.h>
#define _SYS_TYPES_H 1
#define _TIME_H
#include <sys/param.h>
#include <signal.h>
#include <stdlib.h>

#include "command.h"
#include "climain.h"
#include "config.h"
#include "filter.h"

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
static void cmd_disable(FILE *out, char *arg) {
	priv = 0;
}

static void cmd_enable(FILE *out, char *arg) {
	priv = 1;
}

static void cmd_conf_t(FILE *out, char *arg) {
	cmd_root = &command_root_config;
	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	signal(SIGTSTP, swcli_sig_term);
}

void cmd_help(FILE *out, char *arg) {
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

static void cmd_history(FILE *out, char *arg) {
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

static void cmd_exit(FILE *out, char *arg) {
	exit(0);
}

int build_int_eth_cfg(FILE *out, int num) {
	char if_name[IFNAMSIZ];
	struct net_switch_ioctl_arg ioctl_arg;

	sprintf(if_name, "eth%d", num);
	ioctl_arg.cmd = SWCFG_GETIFCFG;
	ioctl_arg.if_name = if_name;

	fprintf(out, "test\n");
	return 0;
}

static void cmd_int_eth(FILE *out, char *arg) {
	int status;
	FILE *tmp;
	char tmp_name[MAXPATHLEN];

	fprintf(out, "Building configuration...\n");
	fflush(out);
	do {
		tmp = mk_tmp_stream(tmp_name, "w+");
		if(tmp == NULL)
			break;
		status = build_int_eth_cfg(tmp, parse_eth(arg));
		if(status)
			break;
		fprintf(out, "\nCurrent configuration:\n");
		rewind(tmp);
		copy_data(out, tmp);
		fclose(tmp);
		unlink(tmp_name);
		fprintf(out, "end\n\n");
		fflush(out);
		return;
	} while(0);
	fprintf(out, "Oops! Something went terribly wrong :(\n");
	return;
}

static void cmd_int_vlan(FILE *out, char *arg) {
}

/* Validation Handlers Implementation */
int valid_regex(char *arg) {
	return 1;
}

int valid_eth(char *arg) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 0 || no > 7) /* FIXME max value */
		return 0;
	/* FIXME interfata e in switch */
	return 1;
}

int valid_vlan(char *arg) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 1 || no > 4094)
		return 0;
	return 1;
}

char vlan_range[] = "<1-1094>\0";

int parse_eth(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);

	return no;
}

int parse_vlan(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);
	assert(no >= 1 && no <= 4094);
	return no;
}

static sw_command_t sh_pipe_regex[] = {
	{"LINE",				0,  valid_regex,	NULL,			RUNNABLE,	"Regular Expression",								NULL},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_pipe_mod[] = {
	{"begin",				0,  NULL,			NULL,			MODE_BEGIN,	"Begin with the line that matches",					sh_pipe_regex},
	{"exclude",				0,  NULL,			NULL,			MODE_EXCLUDE, "Exclude lines that match",						sh_pipe_regex},
	{"include",				0,  NULL,			NULL,			MODE_INCLUDE, "Include lines that match",						sh_pipe_regex},
	{"grep",				0,  NULL,			NULL,			MODE_GREP, "Linux grep functionality",							sh_pipe_regex},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

sw_command_t sh_pipe[] = {
	{"|",					0,  NULL,			NULL,			INCOMPLETE, "Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_int_eth[] = {
	{eth_range,				1,	valid_eth,		cmd_int_eth,	RUNNABLE,	"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_int_vlan[] = {
	{vlan_range,			1,	valid_vlan,		cmd_int_vlan,	RUNNABLE,	"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_sh_run_int[] = {
	{"ethernet",			1,	NULL,			NULL,			INCOMPLETE, "Ethernet IEEE 802.3",								sh_int_eth},
	{"vlan",				1,	NULL,			NULL,			INCOMPLETE, "LMS Vlans",										sh_int_vlan},
	{NULL,					0,	NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_show_run[] = {
	{"interface",			0,	NULL,			NULL,			INCOMPLETE,	"Show interface configuration",						sh_sh_run_int},
	{"|",					0,  NULL,			NULL,			INCOMPLETE, "Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_show[] = {
	{"arp",					0,	NULL,			NULL,			RUNNABLE,	"ARP table",										NULL},
	{"clock",				0,	NULL,			NULL,			RUNNABLE,	"Display the system clock",							NULL},
	{"history",				0,	NULL,			cmd_history,	RUNNABLE,	"Display the session command history",				sh_pipe},
	{"interfaces",			0,	NULL,			NULL,			RUNNABLE,	"Interface status and configuration",				NULL},
	{"ip",					0,	NULL,			NULL,			RUNNABLE,	"IP information",									NULL},
	{"mac",					0,	NULL,			NULL,			RUNNABLE,	"MAC configuration",								NULL},
	{"mac-address-table",	0,	NULL,			NULL,			RUNNABLE,	"MAC forwarding table",								NULL},
	{"running-config",		1,	NULL,			NULL,			RUNNABLE,	"Current operating configuration",					sh_show_run},
	{"sessions",			0,	NULL,			NULL,			RUNNABLE,	"Information about Telnet connections",				NULL},
	{"startup-config",		0,	NULL,			NULL,			RUNNABLE,	"Contents of startup configuration",				NULL},
	{"users",				0,	NULL,			NULL,			RUNNABLE,	"Display information about terminal lines",			NULL},
	{"version",				0,	NULL,			NULL,			RUNNABLE,	"System hardware and software status",				NULL},
	{"vlan",				0,	NULL,			NULL,			RUNNABLE,	"VTP VLAN status",									NULL},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_conf[] = {
	{"terminal",			1,	NULL,			cmd_conf_t,		RUNNABLE,	"Configure from the terminal",						NULL},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh[] = {
	{"clear",				0,	NULL, 			NULL,			INCOMPLETE, "Reset functions",									NULL},
	{"configure",			1,	NULL,			NULL,			INCOMPLETE, "Enter configuration mode",							sh_conf},
	{"disable",				1,	NULL,			cmd_disable,	RUNNABLE,	"Turn off privileged commands",						NULL},
	{"enable",				0,	NULL,			cmd_enable,		RUNNABLE,	"Turn on privileged commands",						NULL},
	{"exit",				0,	NULL,			cmd_exit,		RUNNABLE,	"Exit from the EXEC",								NULL},
	{"help",				0,	NULL,			cmd_help,		RUNNABLE,	"Description of the interactive help system",		NULL},
	{"logout",				0,	NULL,			cmd_exit,		RUNNABLE,	"Exit from the EXEC",								NULL},
	{"ping",				0,	NULL,			NULL,			INCOMPLETE, "Send echo messages",								NULL},
	{"quit",				0,	NULL,			cmd_exit,		RUNNABLE,	"Exit from the EXEC",								NULL},
	{"show",				0,	NULL,			NULL,			INCOMPLETE,	"Show running system information",					sh_show},
	{"telnet",				0,	NULL,			NULL,			INCOMPLETE,	"Open a telnet connection",							NULL},
	{"terminal",			0,	NULL,			NULL,			INCOMPLETE, "Set terminal line parameters",						NULL},
	{"traceroute",			0,	NULL,			NULL,			INCOMPLETE,	"Trace route to destination",						NULL},
	{"where",				0,	NULL,			NULL,			RUNNABLE,	"List active connections",							NULL},
	{NULL,					0,  NULL,			NULL,			NA,			NULL,												NULL}
};

sw_command_root_t command_root_main =					{"%s%c",					sh};
sw_command_root_t command_root_config_vlan =			{"%s(config-vlan)%c",		NULL};

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
	pclose(out);
	exit(0);
}

static void cmd_run_eth(FILE *out, char *arg) {
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

static void cmd_show_run(FILE *out, char *arg) {
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

static void cmd_run_vlan(FILE *out, char *arg) {
}

static void cmd_sh_addr(FILE *out, char *arg) {
}

static void cmd_mac_eth(FILE *out, char *arg) {
}

static void cmd_mac_vlan(FILE *out, char *arg) {
}

static void cmd_sh_dynamic(FILE *out, char *arg) {
}

static void cmd_sh_static(FILE *out, char *arg) {
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

int valid_mac(char *arg) {
	unsigned short a0, a1, a2;

	return (sscanf(arg, "%hx.%hx.%hx", &a0, &a1, &a2) == ETH_ALEN/2);
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

int parse_mac(char *arg, char **mac) {
	unsigned char *buf = calloc(sizeof(unsigned char), ETH_ALEN+1);
	unsigned short a0, a1, a2;

	assert(buf); /* enough memory */
	if (sscanf(arg, "%hx.%hx.%hx", &a0, &a1, &a2) != ETH_ALEN/2) {
		free(buf);
		*mac = NULL;
	}

	buf[0] = a0 / 0x100;
	buf[1] = a0 % 0x100;
	buf[2] = a1 / 0x100;
	buf[3] = a1 % 0x100;
	buf[4] = a2 / 0x100;
	buf[5] = a2 % 0x100;

	*mac = buf;

	return 0;
}

static sw_command_t sh_pipe_regex[] = {
	{"LINE",				0,  valid_regex,	NULL,			RUN,		"Regular Expression",								NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_pipe_mod[] = {
	{"begin",				0,  NULL,			NULL,			MODE_BEGIN,		"Begin with the line that matches",				sh_pipe_regex},
	{"exclude",				0,  NULL,			NULL,			MODE_EXCLUDE,	"Exclude lines that match",						sh_pipe_regex},
	{"include",				0,  NULL,			NULL,			MODE_INCLUDE,	"Include lines that match",						sh_pipe_regex},
	{"grep",				0,  NULL,			NULL,			MODE_GREP,		"Linux grep functionality",						sh_pipe_regex},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

sw_command_t sh_pipe[] = {
	{"|",					0,  NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_int_eth[] = {
	{eth_range,				0,	valid_eth,		cmd_int_eth,	RUN,		"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_int_vlan[] = {
	{vlan_range,			0,	valid_vlan,		cmd_int_vlan,	RUN,		"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_run_eth[] = {
	{eth_range,				1,	valid_eth,		cmd_run_eth,	RUN,		"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_run_vlan[] = {
	{vlan_range,			1,	valid_vlan,		cmd_run_vlan,	RUN,		"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_eth[] = {
	{eth_range,				0,	valid_eth,		cmd_mac_eth,	RUN,		"Ethernet interface number",						NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_vlan[] = {
	{vlan_range,			0,	valid_vlan,		cmd_mac_vlan,	RUN,		"Vlan interface number",							NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_int[] = {
	{"ethernet",			0,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_int_eth},
	{"vlan",				0,	NULL,			NULL,			0,			"LMS Vlans",										sh_int_vlan},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_run_int[] = {
	{"ethernet",			1,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_run_eth},
	{"vlan",				1,	NULL,			NULL,			0,			"LMS Vlans",										sh_run_vlan},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_mac_int[] = {
	{"ethernet",			0,	NULL,			NULL,			0,			"Ethernet IEEE 802.3",								sh_mac_eth},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_sh_mac_addr[] = {
	{"H.H.H",				0,	valid_mac,		cmd_sh_addr,	RUN, 		"48 bit mac address",								NULL},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mac_addr_table[] = {
	{"address",				0,	NULL,			NULL,			0,			"address keyword",									sh_sh_mac_addr},
	{"aging-time",			0,	NULL,			NULL,			RUN,		"aging-time keyword",								NULL},
	{"dynamic",				0,	NULL,			cmd_sh_dynamic,	RUN,		"dynamic entry type",								NULL},
	{"interface",			0,	NULL,			NULL,			0,			"interface keyword",								sh_sh_mac_int},
	{"static",				0,	NULL,			cmd_sh_static,	RUN,		"static entry type",								NULL},
	{"vlan",				0,	NULL,			NULL,			0,			 "VLAN keyword",										sh_mac_vlan},
	{"|",					0,	NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,	NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_show_run[] = {
	{"interface",			0,	NULL,			NULL,			0,			"Show interface configuration",						sh_sh_run_int},
	{"|",					0,  NULL,			NULL,			0,			"Output modifiers",									sh_pipe_mod},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_show[] = {
	{"arp",					0,	NULL,			NULL,			RUN,		"ARP table",										NULL},
	{"clock",				0,	NULL,			NULL,			RUN,		"Display the system clock",							NULL},
	{"history",				0,	NULL,			cmd_history,	RUN,		"Display the session command history",				sh_pipe},
	{"interfaces",			0,	NULL,			cmd_sh_int,		RUN,		"Interface status and configuration",				sh_sh_int},
	{"ip",					0,	NULL,			NULL,			RUN,		"IP information",									NULL},
	{"mac",					0,	NULL,			NULL,			RUN,		"MAC configuration",								NULL},
	{"mac-address-table",	0,	NULL,			NULL,			RUN,		"MAC forwarding table",								sh_mac_addr_table},
	{"running-config",		1,	NULL,			cmd_show_run,	RUN,		"Current operating configuration",					sh_show_run},
	{"sessions",			0,	NULL,			NULL,			RUN,		"Information about Telnet connections",				NULL},
	{"startup-config",		0,	NULL,			NULL,			RUN,		"Contents of startup configuration",				NULL},
	{"users",				0,	NULL,			NULL,			RUN,		"Display information about terminal lines",			NULL},
	{"version",				0,	NULL,			NULL,			RUN,		"System hardware and software status",				NULL},
	{"vlan",				0,	NULL,			NULL,			RUN,		"VTP VLAN status",									NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_conf[] = {
	{"terminal",			1,	NULL,			cmd_conf_t,		RUN,		"Configure from the terminal",						NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

static sw_command_t sh[] = {
	{"clear",				0,	NULL, 			NULL,			0,			"Reset functions",									NULL},
	{"configure",			1,	NULL,			NULL,			0,			"Enter configuration mode",							sh_conf},
	{"disable",				1,	NULL,			cmd_disable,	RUN,		"Turn off privileged commands",						NULL},
	{"enable",				0,	NULL,			cmd_enable,		RUN,		"Turn on privileged commands",						NULL},
	{"exit",				0,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"help",				0,	NULL,			cmd_help,		RUN,		"Description of the interactive help system",		NULL},
	{"logout",				0,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"ping",				0,	NULL,			NULL,			0,			"Send echo messages",								NULL},
	{"quit",				0,	NULL,			cmd_exit,		RUN,		"Exit from the EXEC",								NULL},
	{"show",				0,	NULL,			NULL,			0,			"Show running system information",					sh_show},
	{"telnet",				0,	NULL,			NULL,			0,			"Open a telnet connection",							NULL},
	{"terminal",			0,	NULL,			NULL,			0,			"Set terminal line parameters",						NULL},
	{"traceroute",			0,	NULL,			NULL,			0,			"Trace route to destination",						NULL},
	{"where",				0,	NULL,			NULL,			RUN,		"List active connections",							NULL},
	{NULL,					0,  NULL,			NULL,			0,			NULL,												NULL}
};

sw_command_root_t command_root_main =					{"%s%c",					sh};
sw_command_root_t command_root_config_vlan =			{"%s(config-vlan)%c",		NULL};

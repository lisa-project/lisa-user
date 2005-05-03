#include "command.h"
#include "climain.h"
#include "config.h"
#include "filter.h"

sw_command_t shell_pipe_regex[] = {
	{"LINE",				0,  valid_regex,	NULL,				RUNNABLE,	"Regular Expression",								NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, UNAVAILABLE, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_pipe_mod[] = {
	{"begin",				0,  NULL,			NULL,				MODE_BEGIN,	"Begin with the line that matches",					shell_pipe_regex},
	{"exclude",				0,  NULL,			NULL,				MODE_EXCLUDE, "Exclude lines that match",							shell_pipe_regex},
	{"include",				0,  NULL,			NULL,				MODE_INCLUDE, "Include lines that match",							shell_pipe_regex},
	{"grep",				0,  NULL,			NULL,				MODE_GREP, "Linux grep functionality",							shell_pipe_regex},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, UNAVAILABLE, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_pipe[] = {
	{"|",					0,  NULL,			NULL,				INCOMPLETE, "Output modifiers",									shell_pipe_mod},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, UNAVAILABLE, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main_show[] = {
	{"arp",					0,	NULL,			NULL,				RUNNABLE,	"ARP table",										NULL},
	{"clock",				0,	NULL,			NULL,				RUNNABLE,	"Display the system clock",							NULL},
	{"history",				0,	NULL,			cmd_history,		RUNNABLE,	"Display the session command history",				shell_pipe},
	{"interfaces",			0,	NULL,			NULL,				RUNNABLE,	"Interface status and configuration",				NULL},
	{"ip",					0,	NULL,			NULL,				RUNNABLE,	"IP information",									NULL},
	{"mac",					0,	NULL,			NULL,				RUNNABLE,	"MAC configuration",								NULL},
	{"mac-address-table",	0,	NULL,			NULL,				RUNNABLE,	"MAC forwarding table",								NULL},
	{"sessions",			0,	NULL,			NULL,				RUNNABLE,	"Information about Telnet connections",				NULL},
	{"startup-config",		0,	NULL,			NULL,				RUNNABLE,	"Contents of startup configuration",				NULL},
	{"users",				0,	NULL,			NULL,				RUNNABLE,	"Display information about terminal lines",			NULL},
	{"version",				0,	NULL,			NULL,				RUNNABLE,	"System hardware and software status",				NULL},
	{"vlan",				0,	NULL,			NULL,				RUNNABLE,	"VTP VLAN status",									NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, UNAVAILABLE, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main_conf[] = {
	{"terminal",			1,	NULL,			cmd_conf_t,			RUNNABLE,	"Configure from the terminal",						NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, 0, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main[] = {
	{"clear",				0,	NULL, 			NULL,				INCOMPLETE, "Reset functions",									NULL},
	{"configure",			1,	NULL,			NULL,				INCOMPLETE, "Enter configuration mode",							shell_main_conf},
	{"disable",				1,	NULL,			cmd_disable,		RUNNABLE,	"Turn off privileged commands",						NULL},
	{"enable",				0,	NULL,			cmd_enable,			RUNNABLE,	"Turn on privileged commands",						NULL},
	{"exit",				0,	NULL,			cmd_exit,			RUNNABLE,	"Exit from the EXEC",								NULL},
	{"help",				0,	NULL,			cmd_help,			RUNNABLE,	"Description of the interactive help system",		NULL},
	{"logout",				0,	NULL,			cmd_exit,			RUNNABLE,	"Exit from the EXEC",								NULL},
	{"ping",				0,	NULL,			NULL,				INCOMPLETE, "Send echo messages",								NULL},
	{"quit",				0,	NULL,			cmd_exit,			RUNNABLE,	"Exit from the EXEC",								NULL},
	{"show",				0,	NULL,			NULL,				INCOMPLETE,	"Show running system information",					shell_main_show},
	{"telnet",				0,	NULL,			NULL,				INCOMPLETE,	"Open a telnet connection",							NULL},
	{"terminal",			0,	NULL,			NULL,				INCOMPLETE, "Set terminal line parameters",						NULL},
	{"traceroute",			0,	NULL,			NULL,				INCOMPLETE,	"Trace route to destination",						NULL},
	{"where",				0,	NULL,			NULL,				RUNNABLE,	"List active connections",							NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, 0, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root_main =					{"%s%c",					shell_main};
sw_command_root_t command_root_config_if =				{"%s(config-if)%c",			NULL};
sw_command_root_t command_root_config_vlan =			{"%s(config-vlan)%c",		NULL};

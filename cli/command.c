#include "command.h"
#include "climain.h"
#include "config.h"

sw_command_t shell_main_show[] = {
	{"arp",					0,	NULL,		NULL,					"ARP table",										NULL},
	{"clock",				0,	NULL,		NULL,					"Display the system clock",							NULL},
	{"history",				0,	NULL,		NULL,					"Display the session command history",				NULL},
	{"interfaces",			0,	NULL,		NULL,					"Interface status and configuration",				NULL},
	{"ip",					0,	NULL,		NULL,					"IP information",									NULL},
	{"mac",					0,	NULL,		NULL,					"MAC configuration",								NULL},
	{"mac-address-table",	0,	NULL,		NULL,					"MAC forwarding table",								NULL},
	{"sessions",			0,	NULL,		NULL,					"Information about Telnet connections",				NULL},
	{"startup-config",		0,	NULL,		NULL,					"Contents of startup configuration",				NULL},
	{"users",				0,	NULL,		NULL,					"Display information about terminal lines",			NULL},
	{"version",				0,	NULL,		NULL,					"System hardware and software status",				NULL},
	{"vlan",				0,	NULL,		NULL,					"VTP VLAN status",									NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main_conf[] = {
	{"terminal",			1,	NULL,		cmd_conf_t,				"Configure from the terminal",						NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main[] = {
	{"clear",				0,	NULL, 		NULL,					"Reset functions",									NULL},
	{"configure",			1,	NULL,		NULL,					"Enter configuration mode",							shell_main_conf},
	{"disable",				1,	NULL,		cmd_disable,			"Turn off privileged commands",						NULL},
	{"enable",				0,	NULL,		cmd_enable,				"Turn on privileged commands",						NULL},
	{"exit",				0,	NULL,		cmd_exit,				"Exit from the EXEC",								NULL},
	{"help",				0,	NULL,		cmd_help,				"Description of the interactive help system",		NULL},
	{"logout",				0,	NULL,		cmd_exit,				"Exit from the EXEC",								NULL},
	{"ping",				0,	NULL,		NULL,					"Send echo messages",								NULL},
	{"quit",				0,	NULL,		cmd_exit,				"Exit from the EXEC",								NULL},
	{"show",				0,	NULL,		NULL,					"Show running system information",					shell_main_show},
	{"telnet",				0,	NULL,		NULL,					"Open a telnet connection",							NULL},
	{"terminal",			0,	NULL,		NULL,					"Set terminal line parameters",						NULL},
	{"traceroute",			0,	NULL,		NULL,					"Trace route to destination",						NULL},
	{"where",				0,	NULL,		NULL,					"List active connections",							NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root_main =					{"%s%c",					shell_main};
sw_command_root_t command_root_config_if =				{"%s(config-if)%c",			NULL};
sw_command_root_t command_root_config_vlan =			{"%s(config-vlan)%c",		NULL};

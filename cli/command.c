#include "command.h"
#include "climain.h"

sw_command_t shell_main_show[] = {
	{"arp",					0,	NULL,			"ARP table",		NULL},
	{"clock",				0,	NULL,			"Display the system clock", NULL},
	{"history",				0,	NULL,			"Display the session command history", NULL},
	{"interfaces",			0,	NULL,			"Interface status and configuration", NULL},
	{"ip",					0,	NULL,			"IP information", NULL},
	{"mac",					0,	NULL,			"MAC configuration", NULL},
	{"mac-address-table",	0,	NULL,			"MAC forwarding table", NULL},
	{"sessions",			0,	NULL,			"Information about Telnet connections", NULL},
	{"startup-config",		0,	NULL,			"Contents of startup configuration", NULL},
	{"users",				0,	NULL,			"Display information about terminal lines", NULL},
	{"version",				0,	NULL,			"System hardware and software status", NULL},
	{"vlan",				0,	NULL,			"VTP VLAN status", NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main[] = {
	{"clear",				0,	NULL, 			"Reset functions", NULL},
	{"clock",				0,	NULL,			"Manage the system clock", NULL},
	{"configure",			1,	NULL,			"Enter configuration mode", NULL},
	{"disable",				1,	NULL,			"Turn off privileged commands", NULL},
	{"enable",				0,	cmd_enable,		"Turn on privileged commands", NULL},
	{"exit",				0,	cmd_exit,		"Exit from the EXEC", NULL},
	{"help",				0,	cmd_help,		"Description of the interactive help system", NULL},
	{"logout",				0,	cmd_exit,		"Exit from the EXEC", NULL},
	{"ping",				0,	NULL,			"Send echo messages", NULL},
	{"reload",				1,	NULL,			"Halt and perform a cold restart", NULL},
	{"show",				0,	NULL,			"Show running system information", shell_main_show},
	{"traceroute",			0,	NULL,			"Trace route to destination", NULL},
	{"vlan",				1,	NULL,			"Configure VLAN parameters", NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root[] = {
    {"%s%c",					shell_main},
	{NULL,						NULL}
};

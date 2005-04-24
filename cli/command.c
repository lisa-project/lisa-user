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
	{"version",				0,	NULL,			"System hardware and software status", NULL },
	{"vlan",				0,	NULL,			"VTP VLAN status", NULL },
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_main[] = {
	{"clear",		0,	NULL, 			"Reset functions", NULL },
	{"disable",		0,	NULL,			"Turn off privileged commands", NULL},
	{"enable",		0,	cmd_enable,		"Turn on privileged commands", NULL },
	{"exit",		0,	cmd_exit,		"Exit from the EXEC", NULL},
	{"help",		0,	cmd_help,		"Description of the interactive help system", NULL},
	{"logout", 0, cmd_exit, "Exit from the EXEC", NULL },
	{"name-connection", 0, NULL, "Name an existing network connection", NULL},
	{"ping", 0, NULL, "Send echo messages", NULL },
	{"set", 0, NULL, "Set system parameter (not config)", NULL},
	{"show", 0, NULL, "Show running system information", shell_main_show},
	{"systat", 0, NULL, "Display information about terminal lines", NULL},
	{"telnet", 0, NULL, "Open a telnet connection", NULL},
	{"terminal", 0, NULL, "Set terminal line parameters", NULL},
	{"traceroute", 0, NULL, "Trace route to destination", NULL},
	{"where", 0, NULL, "List active connections", NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_config[] = {
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_config_if[] = {
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_config_vlan[] = {
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_t shell_vlan[] = {
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root[] = {
    {"%s%c",					shell_main},
	{"%s(config)%c",			shell_config},
	{"%s(config-if)%c",			shell_config_if},
	{"%s(config-vlan)%c",		shell_config_vlan},
	{"%s(vlan)%c",				shell_vlan},
	{NULL,						NULL}
};

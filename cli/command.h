#ifndef _COMMAND_H
#define _COMMAND_H


/* FIXME: Initially some commands in here will be bogus */

SW_COMMAND_T sh_unpriv_commands[] = {
	{ "clock", NULL, "Display the system clock", NULL},
	{ "history", NULL, "Display the session command history", NULL},
	{ "sessions", NULL, "Information about Telnet connections", NULL},
	{ "spanning-tree", NULL, "Spanning tree topology", NULL},
	{ "users", NULL, "Display information about terminal lines", NULL},
	{ "version", NULL, "System hardware and software status", NULL },
	{ "vlan", NULL, "VTP VLAN status", NULL },
	{ "vtp", NULL, "VTP information", NULL },
	{ (char *)NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (SW_COMMAND_T *) NULL}
};

SW_COMMAND_T main_unpriv_commands[] = {
	{ "clear", NULL, "Reset functions", NULL },
	{ "disable", NULL, "Turn off privileged commands", NULL},
	{ "enable", cmd_enable, "Turn on privileged commands", NULL },
	{ "exit", cmd_exit, "Exit from the EXEC", NULL},
	{ "help", cmd_help, "Description of the interactive help system", NULL},
	{ "logout", cmd_exit, "Exit from the EXEC", NULL },
	{ "name-connection", NULL, "Name an existing network connection", NULL},
	{ "ping", NULL, "Send echo messages", NULL },
	{ "set", NULL, "Set system parameter (not config)", NULL},
	{ "show", NULL, "Show running system information", sh_unpriv_commands },
	{ "systat", NULL, "Display information about terminal lines", NULL},
	{ "telnet", NULL, "Open a telnet connection", NULL},
	{ "terminal", NULL, "Set terminal line parameters", NULL},
	{ "traceroute", NULL, "Trace route to destination", NULL},
	{ "where", NULL, "List active connections", NULL},
	{ (char *)NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (SW_COMMAND_T *) NULL}
};

#endif

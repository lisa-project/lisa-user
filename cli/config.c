#include "command.h"
#include "climain.h"

int cmd_conf_end(char *arg) {
	cmd_root = &command_root_main;
	/* FIXME scoatere binding ^Z */
	return 0;
}

int cmd_conf_hostname(char *arg) {
	return 0;
}

sw_command_t shell_conf[] = {
	{"enable",				1,	NULL,		NULL,					"Modify enable password parameters",				NULL},
	{"end",					1,	NULL,		cmd_conf_end,			"Exit from configure mode",							NULL},
	{"exit",				1,	NULL,		cmd_conf_end,			"Exit from configure mode",							NULL},
	{"hostname",			1,	NULL,		cmd_conf_hostname,		"Set system's network name",						NULL},
	{"interface",			1,	NULL,		NULL,					"Select an interface to configure",					NULL},
	{"mac-address-table",	1,	NULL,		NULL,					"Configure the MAC address table",					NULL},
	{"no",					1,	NULL,		NULL,					"Negate a command or set its defaults",				NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			shell_conf};

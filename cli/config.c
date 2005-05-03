#include "command.h"
#include "climain.h"

void cmd_conf_end(FILE *out, char *arg) {
	cmd_root = &command_root_main;
	/* FIXME scoatere binding ^Z */
}

void cmd_conf_hostname(FILE *out, char *arg) {
}

sw_command_t shell_conf[] = {
	{"enable",				1,	NULL,		NULL,					INCOMPLETE, "Modify enable password parameters",				NULL},
	{"end",					1,	NULL,		cmd_conf_end,			RUNNABLE,	"Exit from configure mode",							NULL},
	{"exit",				1,	NULL,		cmd_conf_end,			RUNNABLE,	"Exit from configure mode",							NULL},
	{"hostname",			1,	NULL,		cmd_conf_hostname,		INCOMPLETE,	"Set system's network name",						NULL},
	{"interface",			1,	NULL,		NULL,					INCOMPLETE,	"Select an interface to configure",					NULL},
	{"mac-address-table",	1,	NULL,		NULL,					INCOMPLETE,	"Configure the MAC address table",					NULL},
	{"no",					1,	NULL,		NULL,					INCOMPLETE, "Negate a command or set its defaults",				NULL},
	{(char *)NULL, 0, (rl_icpfunc_t *) NULL, (sw_command_handler) NULL, UNAVAILABLE, (char *) NULL, (sw_command_t *) NULL}
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			shell_conf};

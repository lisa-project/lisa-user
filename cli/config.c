#include "command.h"
#include "climain.h"
#include "config_if.h"

static void cmd_end(FILE *out, char *arg) {
	cmd_root = &command_root_main;
	/* FIXME scoatere binding ^Z */
}

static void cmd_hostname(FILE *out, char *arg) {
}

static void cmd_int_eth(FILE *out, char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);

	eth_no = no;
	cmd_root = &command_root_config_if_eth;
}

static void cmd_int_vlan(FILE *out, char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);
	assert(no >= 1 && no <= 4094);

	vlan_no = no;
	cmd_root = &command_root_config_if_vlan;
}

static int valid_eth(char *arg) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 0 || no > 7) /* FIXME max value */
		return 0;
	/* FIXME interfata e in switch */
	return 1;
}

static int valid_vlan(char *arg) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 1 || no > 4094)
		return 0;
	return 1;
}

static sw_command_t sh_int_eth[] = {
	{eth_range,				1,	valid_eth,	cmd_int_eth,		RUNNABLE,	"Ethernet interface number",					NULL},
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

static sw_command_t sh_int_vlan[] = {
	{"<1-4094>",			1,	valid_vlan,	cmd_int_vlan,		RUNNABLE,	"Vlan interface number",						NULL},
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

static sw_command_t sh_int[] = {
	{"ethernet",			1,	NULL,		NULL,				INCOMPLETE, "Ethernet IEEE 802.3",							sh_int_eth},
	{"vlan",				1,	NULL,		NULL,				INCOMPLETE, "LMS Vlans",									sh_int_vlan},
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

static sw_command_t sh[] = {
	{"enable",				1,	NULL,		NULL,				INCOMPLETE, "Modify enable password parameters",			NULL},
	{"end",					1,	NULL,		cmd_end,			RUNNABLE,	"Exit from configure mode",						NULL},
	{"exit",				1,	NULL,		cmd_end,			RUNNABLE,	"Exit from configure mode",						NULL},
	{"hostname",			1,	NULL,		cmd_hostname,		INCOMPLETE,	"Set system's network name",					NULL},
	{"interface",			1,	NULL,		NULL,				INCOMPLETE,	"Select an interface to configure",				sh_int},
	{"mac-address-table",	1,	NULL,		NULL,				INCOMPLETE,	"Configure the MAC address table",				NULL},
	{"no",					1,	NULL,		NULL,				INCOMPLETE, "Negate a command or set its defaults",			NULL},
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			sh};

#include <linux/if.h>
/* Hack to get around conflicting linux/types.h and sys/types.h */
#define _SYS_TYPES_H 1
#include <linux/sockios.h>
#include <linux/net_switch.h>

#include <sys/ioctl.h>

#include "climain.h"
#include "config.h"
#include "config_if.h"

char sel_eth[IFNAMSIZ];
char sel_vlan[IFNAMSIZ];
int int_type;

static void cmd_exit(FILE *out, char *arg) {
	cmd_root = &command_root_config;
}

static void cmd_acc_vlan(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.name = sel_eth;
	ioctl_arg.vlan = parse_vlan(arg);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static sw_command_t sh_acc_vlan[] = {
	{vlan_range,			0,	valid_vlan,	cmd_acc_vlan,	RUNNABLE,	"VLAN ID of the VLAN when this port is in access mode",	NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_access[] = {
	{"vlan",				0,	NULL,		NULL,			INCOMPLETE,	"Set VLAN when interface is in access mode",		sh_acc_vlan},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_mode[] = {
	{"access",				1,	NULL,		NULL,			RUNNABLE,	"Set trunking mode to ACCESS unconditionally",		NULL},
	{"trunk",				1,	NULL,		NULL,			RUNNABLE,	"Set trunking mode to TRUNK unconditionally",		NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_all_vlan[] = {
	{"WORD",				1,	NULL,		NULL,			INCOMPLETE,	"VLAN IDs of the allowed VLANs when this port is in trunking mode",	NULL},
	{"add",					1,	NULL,		NULL,			INCOMPLETE,	"add VLANs to the current list",					NULL},
	{"all",					1,	NULL,		NULL,			RUNNABLE,	"all VLANs",										NULL},
	{"except",				1,	NULL,		NULL,			INCOMPLETE,	"all VLANs except the following",					NULL},
	{"none",				1,	NULL,		NULL,			RUNNABLE,	"no VLANs",											NULL},
	{"remove",				1,	NULL,		NULL,			INCOMPLETE,	"remove VLANs from the current list",				NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_allowed[] = {
	{"vlan",				1,	NULL,		NULL,			INCOMPLETE,	"Set allowed VLANs when interface is in trunking mode",	sh_all_vlan},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_trunk[] = {
	{"allowed",				1,	NULL,		NULL,			INCOMPLETE,	"Set allowed VLAN characteristics when interface is in trunking mode",	sh_allowed},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_switchport[] = {
	{"access",				1,	NULL,		NULL,			INCOMPLETE,	"Set access mode characteristics of the interface",	sh_access},
	{"mode",				1,	NULL,		NULL,			INCOMPLETE,	"Set trunking mode of the interface",				sh_mode},
	{"trunk",				1,	NULL,		NULL,			INCOMPLETE,	"Set trunking characteristics of the interface",	sh_trunk},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_eth[] = {
	{"description",			1,	NULL,		NULL,			INCOMPLETE,	"Interface specific description",					NULL},
	{"duplex",				1,	NULL,		NULL,			INCOMPLETE,	"Configure duplex operation.",						NULL},
	{"exit",				1,	NULL,		cmd_exit,		RUNNABLE,	"Exit from interface configuration mode",			NULL},
	{"help",				1,	NULL,		NULL,			RUNNABLE,	"Description of the interactive help system",		NULL},
	{"no",					1,	NULL,		NULL,			INCOMPLETE,	"Negate a command or set its defaults",				NULL},
	{"shutdown",			1,	NULL,		NULL,			RUNNABLE,	"Shutdown the selected interface",					NULL},
	{"speed",				1,	NULL,		NULL,			INCOMPLETE,	"Configure speed operation.",						NULL},
	{"switchport",			1,	NULL,		NULL,			INCOMPLETE,	"Set switching mode characteristics",				sh_switchport},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_vlan[] = {
	{NULL,					0,	NULL,		NULL,			NA,			NULL,											NULL}
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};

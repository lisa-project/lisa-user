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
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.vlan = parse_vlan(arg);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noacc_vlan(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.vlan = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_shutd(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_DISABLEPORT;
	ioctl_arg.if_name = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noshutd(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_ENABLEPORT;
	ioctl_arg.if_name = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_access(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETACCESS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.access = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_trunk(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.vlan = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_nomode(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.trunk = 0;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	ioctl_arg.cmd = SWCFG_SETACCESS;
	ioctl_arg.ext.access = 0;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')

static int valid_vlst(char *arg) {
	return is_digit(*arg);
}

static int parse_vlan_list(char *list, unsigned char *bmp) {
	int state = 0;
	int min, max;
	char *last = list, *ptr;

	memset(bmp, 0xff, SW_VLAN_BMP_NO);
	for(ptr = list; *ptr != '\0'; ptr++) {
		switch(state) {
		case 0: /* First number */
			if(is_digit(*ptr))
				continue;
			switch(*ptr) {
			case '-':
				min = atoi(last);
				if(sw_invalid_vlan(min))
					return 1;
				last = ptr + 1;
				state = 1;
				continue;
			case ',':
				min = atoi(last);
				if(sw_invalid_vlan(min))
					return 1;
				last = ptr + 1;
				sw_allow_vlan(bmp, min);
				continue;
			}
			return 2;
		case 1: /* Second number */
			if(is_digit(*ptr))
				continue;
			if(*ptr == ',') {
				max = atoi(last);
				if(sw_invalid_vlan(max))
					return 1;
				while(min <= max) {
					sw_allow_vlan(bmp, min);
					min++;
				}
				last = ptr + 1;
				state = 0;
			}
			return 2;
		}
	}
	switch(state) {
	case 0:
		min = atoi(last);
		if(sw_invalid_vlan(min))
			return 1;
		sw_allow_vlan(bmp, min);
		break;
	case 1:
		max = atoi(last);
		if(sw_invalid_vlan(max))
			return 1;
		while(min <= max) {
			sw_allow_vlan(bmp, min);
			min++;
		}
		break;
	}
	return 0;
}

#define get_vlan_list if(parse_vlan_list(arg, bmp)) {\
	fprintf(out, "Command rejected: Bad VLAN list\n");\
	fflush(out);\
	return;\
}

static void cmd_setvlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_addvlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_ADDTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_allvlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_excvlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];
	int i;

	get_vlan_list;
	for(i = 0; i < SW_VLAN_BMP_NO; i++)
		bmp[i] ^= 0xff;
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_novlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0xff, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_remvlans(FILE *out, char *arg) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_DELTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

char VLAN_IDs_of_the_allowed_VLANs[] =
"VLAN IDs of the allowed VLANs when this port is in trunking mode\0";

static sw_command_t sh_acc_vlan[] = {
	{vlan_range,			0,	valid_vlan,	cmd_acc_vlan,	RUNNABLE,	"VLAN ID of the VLAN when this port is in access mode",	NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_access[] = {
	{"vlan",				0,	NULL,		NULL,			INCOMPLETE,	"Set VLAN when interface is in access mode",		sh_acc_vlan},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_noaccess[] = {
	{"vlan",				0,	NULL,		cmd_noacc_vlan,	RUNNABLE,	"Set VLAN when interface is in access mode",		NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_mode[] = {
	{"access",				1,	NULL,		cmd_access,		RUNNABLE,	"Set trunking mode to ACCESS unconditionally",		NULL},
	{"trunk",				1,	NULL,		cmd_trunk,		RUNNABLE,	"Set trunking mode to TRUNK unconditionally",		NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_addvlan[] = {
	{"WORD",				1,	valid_vlst,	cmd_addvlans,	RUNNABLE,	VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_excvlan[] = {
	{"WORD",				1,	valid_vlst,	cmd_excvlans,	RUNNABLE,	VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_remvlan[] = {
	{"WORD",				1,	valid_vlst,	cmd_remvlans,	RUNNABLE,	VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_all_vlan[] = {
	{"WORD",				1,	valid_vlst,	cmd_setvlans,	RUNNABLE,	VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{"add",					1,	NULL,		NULL,			INCOMPLETE,	"add VLANs to the current list",					sh_addvlan},
	{"all",					1,	NULL,		cmd_allvlans,	RUNNABLE,	"all VLANs",										NULL},
	{"except",				1,	NULL,		NULL,			INCOMPLETE,	"all VLANs except the following",					sh_excvlan},
	{"none",				1,	NULL,		cmd_novlans,	RUNNABLE,	"no VLANs",											NULL},
	{"remove",				1,	NULL,		NULL,			INCOMPLETE,	"remove VLANs from the current list",				sh_remvlan},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static char Set_allowed_VLANs[] =
"Set allowed VLANs when interface is in trunking mode\0";

static sw_command_t sh_allowed[] = {
	{"vlan",				1,	NULL,		NULL,			INCOMPLETE,	Set_allowed_VLANs,									sh_all_vlan},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_noallowed[] = {
	{"vlan",				1,	NULL,		cmd_allvlans,	RUNNABLE,	Set_allowed_VLANs,									NULL},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static char Set_allowed_VLAN_characteristics[] =
"Set allowed VLAN characteristics when interface is in trunking mode\0";

static sw_command_t sh_trunk[] = {
	{"allowed",				1,	NULL,		NULL,			INCOMPLETE,	Set_allowed_VLAN_characteristics,					sh_allowed},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_notrunk[] = {
	{"allowed",				1,	NULL,		NULL,			INCOMPLETE,	Set_allowed_VLAN_characteristics,					sh_noallowed},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_switchport[] = {
	{"access",				1,	NULL,		NULL,			INCOMPLETE,	"Set access mode characteristics of the interface",	sh_access},
	{"mode",				1,	NULL,		NULL,			INCOMPLETE,	"Set trunking mode of the interface",				sh_mode},
	{"trunk",				1,	NULL,		NULL,			INCOMPLETE,	"Set trunking characteristics of the interface",	sh_trunk},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_noswitchport[] = {
	{"access",				1,	NULL,		NULL,			INCOMPLETE,	"Set access mode characteristics of the interface",	sh_noaccess},
	{"mode",				1,	NULL,		cmd_nomode,		RUNNABLE,	"Set trunking mode of the interface",				NULL},
	{"trunk",				1,	NULL,		NULL,			INCOMPLETE,	"Set trunking characteristics of the interface",	sh_notrunk},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_no[] = {
	{"shutdown",			1,	NULL,		cmd_noshutd,	RUNNABLE,	"Shutdown the selected interface",					NULL},
	{"switchport",			1,	NULL,		NULL,			INCOMPLETE,	"Set switching mode characteristics",				sh_noswitchport},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_eth[] = {
	{"description",			1,	NULL,		NULL,			INCOMPLETE,	"Interface specific description",					NULL},
	{"duplex",				1,	NULL,		NULL,			INCOMPLETE,	"Configure duplex operation.",						NULL},
	{"exit",				1,	NULL,		cmd_exit,		RUNNABLE,	"Exit from interface configuration mode",			NULL},
	{"help",				1,	NULL,		cmd_help,		RUNNABLE,	"Description of the interactive help system",		NULL},
	{"interface",			1,	NULL,		NULL,			INCOMPLETE,	"Select an interface to configure",					sh_conf_int},
	{"no",					1,	NULL,		NULL,			INCOMPLETE,	"Negate a command or set its defaults",				sh_no},
	{"shutdown",			1,	NULL,		cmd_shutd,		RUNNABLE,	"Shutdown the selected interface",					NULL},
	{"speed",				1,	NULL,		NULL,			INCOMPLETE,	"Configure speed operation.",						NULL},
	{"switchport",			1,	NULL,		NULL,			INCOMPLETE,	"Set switching mode characteristics",				sh_switchport},
	{NULL,					0,	NULL,		NULL,			NA,			NULL,												NULL}
};

static sw_command_t sh_vlan[] = {
	{NULL,					0,	NULL,		NULL,			NA,			NULL,											NULL}
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};

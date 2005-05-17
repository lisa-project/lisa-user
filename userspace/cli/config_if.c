#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "climain.h"
#include "config.h"
#include "config_if.h"

char sel_eth[IFNAMSIZ];
char sel_vlan[IFNAMSIZ];
int int_type;

static void cmd_exit(FILE *out, char **argv) {
	cmd_root = &command_root_config;
}

static void cmd_acc_vlan(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.vlan = parse_vlan(arg);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noacc_vlan(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.vlan = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_shutd_v(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_DISABLEVIF;
	sscanf(sel_vlan, "vlan%d", &ioctl_arg.vlan);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noshutd_v(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_ENABLEVIF;
	sscanf(sel_vlan, "vlan%d", &ioctl_arg.vlan);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_shutd(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_DISABLEPORT;
	ioctl_arg.if_name = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noshutd(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_ENABLEPORT;
	ioctl_arg.if_name = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_access(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETACCESS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.access = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_trunk(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.trunk = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_nomode(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.trunk = 0;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	ioctl_arg.cmd = SWCFG_SETACCESS;
	ioctl_arg.ext.access = 0;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static int valid_vlst(char *arg, char lookahead) {
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

static void cmd_setvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_addvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_ADDTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_allvlans(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_excvlans(FILE *out, char **argv) {
	char *arg = *argv;
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

static void cmd_novlans(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0xff, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_remvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_DELTRUNKVLANS;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static int valid_desc(char *argv, char lookahead) {
	return 1;
}

static void cmd_setethdesc(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETIFDESC;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.iface_desc = arg;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noethdesc(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETIFDESC;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.iface_desc = "";
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_10(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_10;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_100(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_100;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_auto(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_AUTO;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_auto(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_AUTO;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_full(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_FULL;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_half(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.if_name = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_HALF;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

char VLAN_IDs_of_the_allowed_VLANs[] =
"VLAN IDs of the allowed VLANs when this port is in trunking mode\0";

static sw_command_t sh_acc_vlan[] = {
	{vlan_range,			1,	valid_vlan,	cmd_acc_vlan,	RUN,		"VLAN ID of the VLAN when this port is in access mode",	NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_access[] = {
	{"vlan",				1,	NULL,		NULL,			0,			"Set VLAN when interface is in access mode",		sh_acc_vlan},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_noaccess[] = {
	{"vlan",				1,	NULL,		cmd_noacc_vlan,	RUN,		"Set VLAN when interface is in access mode",		NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_mode[] = {
	{"access",				2,	NULL,		cmd_access,		RUN,		"Set trunking mode to ACCESS unconditionally",		NULL},
	{"trunk",				2,	NULL,		cmd_trunk,		RUN,		"Set trunking mode to TRUNK unconditionally",		NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_addvlan[] = {
	{"WORD",				2,	valid_vlst,	cmd_addvlans,	RUN,		VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_excvlan[] = {
	{"WORD",				2,	valid_vlst,	cmd_excvlans,	RUN,		VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_remvlan[] = {
	{"WORD",				2,	valid_vlst,	cmd_remvlans,	RUN,		VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_all_vlan[] = {
	{"WORD",				2,	valid_vlst,	cmd_setvlans,	RUN,		VLAN_IDs_of_the_allowed_VLANs,						NULL},
	{"add",					2,	NULL,		NULL,			0,			"add VLANs to the current list",					sh_addvlan},
	{"all",					2,	NULL,		cmd_allvlans,	RUN,		"all VLANs",										NULL},
	{"except",				2,	NULL,		NULL,			0,			"all VLANs except the following",					sh_excvlan},
	{"none",				2,	NULL,		cmd_novlans,	RUN,		"no VLANs",											NULL},
	{"remove",				2,	NULL,		NULL,			0,			"remove VLANs from the current list",				sh_remvlan},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static char Set_allowed_VLANs[] =
"Set allowed VLANs when interface is in trunking mode\0";

static sw_command_t sh_allowed[] = {
	{"vlan",				2,	NULL,		NULL,			0,			Set_allowed_VLANs,									sh_all_vlan},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_noallowed[] = {
	{"vlan",				2,	NULL,		cmd_allvlans,	RUN,		Set_allowed_VLANs,									NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static char Set_allowed_VLAN_characteristics[] =
"Set allowed VLAN characteristics when interface is in trunking mode\0";

static sw_command_t sh_trunk[] = {
	{"allowed",				2,	NULL,		NULL,			0,			Set_allowed_VLAN_characteristics,					sh_allowed},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_notrunk[] = {
	{"allowed",				2,	NULL,		NULL,			0,			Set_allowed_VLAN_characteristics,					sh_noallowed},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_switchport[] = {
	{"access",				2,	NULL,		NULL,			0,			"Set access mode characteristics of the interface",	sh_access},
	{"mode",				2,	NULL,		NULL,			0,			"Set trunking mode of the interface",				sh_mode},
	{"trunk",				2,	NULL,		NULL,			0,			"Set trunking characteristics of the interface",	sh_trunk},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_noswitchport[] = {
	{"access",				2,	NULL,		NULL,			0,			"Set access mode characteristics of the interface",	sh_noaccess},
	{"mode",				2,	NULL,		cmd_nomode,		RUN,		"Set trunking mode of the interface",				NULL},
	{"trunk",				2,	NULL,		NULL,			0,			"Set trunking characteristics of the interface",	sh_notrunk},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_ethdesc[] = {
	{"LINE",				2,	valid_desc,	cmd_setethdesc,	RUN,		"A character string describing this interface",		NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_speed[] = {
	{"10",					2,	NULL,		cmd_sp_10,		RUN,		"Force 10 Mbps operation",							NULL},
	{"100",					2,	NULL,		cmd_sp_100,		RUN,		"Force 100 Mbps operation",							NULL},
	{"auto",				2,	NULL,		cmd_sp_auto,	RUN,		"Enable AUTO speed configuration",					NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_duplex[] = {
	{"auto",				2,	NULL,		cmd_du_auto,	RUN,		"Enable AUTO duplex configuration",					NULL},
	{"full",				2,	NULL,		cmd_du_full,	RUN,		"Force full duplex operation",						NULL},
	{"half",				2,	NULL,		cmd_du_half,	RUN,		"Force half-duplex operation",						NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_no[] = {
	{"description",			2,	NULL,		cmd_noethdesc,	RUN,		"Interface specific description",					NULL},
	{"duplex",				2,	NULL,		cmd_du_auto,	RUN,		"Configure duplex operation.",						NULL},
	{"shutdown",			2,	NULL,		cmd_noshutd,	RUN,		"Shutdown the selected interface",					NULL},
	{"speed",				2,	NULL,		cmd_sp_auto,	RUN,		"Configure speed operation.",						NULL},
	{"switchport",			2,	NULL,		NULL,			0,			"Set switching mode characteristics",				sh_noswitchport},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_no_vlan[] = {
	{"shutdown",			2,	NULL,		cmd_noshutd_v,	RUN,		"Shutdown the selected interface",					NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_eth[] = {
	{"description",			2,	NULL,		NULL,			0,			"Interface specific description",					sh_ethdesc},
	{"duplex",				2,	NULL,		NULL,			0,			"Configure duplex operation.",						sh_duplex},
	{"end"	,				2,	NULL,		cmd_exit,		RUN,		"End interface configuration mode",					NULL},
	{"exit",				2,	NULL,		cmd_exit,		RUN,		"Exit from interface configuration mode",			NULL},
	{"help",				2,	NULL,		cmd_help,		RUN,		"Description of the interactive help system",		NULL},
	{"interface",			2,	NULL,		NULL,			0,			"Select an interface to configure",					sh_conf_int},
	{"no",					2,	NULL,		NULL,			0,			"Negate a command or set its defaults",				sh_no},
	{"shutdown",			2,	NULL,		cmd_shutd,		RUN,		"Shutdown the selected interface",					NULL},
	{"speed",				2,	NULL,		NULL,			0,			"Configure speed operation.",						sh_speed},
	{"switchport",			2,	NULL,		NULL,			0,			"Set switching mode characteristics",				sh_switchport},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_vlan[] = {
	{"end",					2,	NULL,		cmd_exit,		RUN,		"Exit from interface configuration mode",			NULL},
	{"exit",				2,	NULL,		cmd_exit,		RUN,		"End interface configuration mode",					NULL},
	{"help",				2,	NULL,		cmd_help,		RUN,		"Description of the interactive help system",		NULL},
	{"no",					2,	NULL,		NULL,			0,			"Negate a command or set its defaults",				sh_no_vlan},
	{"shutdown",			2,	NULL,		cmd_shutd_v,	RUN,		"Shutdown the selected interface",					NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};

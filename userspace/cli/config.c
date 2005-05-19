#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#define __USE_XOPEN
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>

#include "command.h"
#include "climain.h"
#include "config_if.h"
#include "shared.h"

#include <errno.h>
extern int errno;
char hostname_default[] = "Switch\0";

static char salt_base[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

static void cmd_end(FILE *out, char **argv) {
	cmd_root = &command_root_main;
	/* FIXME scoatere binding ^Z */
}

static void cmd_hostname(FILE *out, char **argv) {
	char *arg = *argv;
	sethostname(arg, strlen(arg));
}

static void cmd_nohostname(FILE *out, char **argv) {
	sethostname(hostname_default, strlen(hostname_default));
}

static void cmd_int_eth(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_ADDIF;
	ioctl_arg.if_name = if_name_eth(arg);
	do {
		if(!ioctl(sock_fd, SIOCSWCFG, &ioctl_arg))
			break;
		if(errno == ENODEV) {
			fprintf(out, "Command rejected: device %s does not exist\n",
					ioctl_arg.if_name);
			return;
		}
	} while(0);

	cmd_root = &command_root_config_if_eth;
	strcpy(sel_eth, ioctl_arg.if_name);
}

static void cmd_no_int_eth(FILE *out, char **argv) {
	char *arg = argv[1]; /* argv[0] is "no" */
	struct net_switch_ioctl_arg ioctl_arg;
	
	ioctl_arg.cmd = SWCFG_DELIF;
	ioctl_arg.if_name = if_name_eth(arg);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_int_vlan(FILE *out, char **argv) {
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	int vlan = parse_vlan(arg);

	ioctl_arg.cmd = SWCFG_ADDVIF;
	ioctl_arg.vlan = vlan;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);

	sprintf(sel_vlan, "vlan%d", vlan);
	cmd_root = &command_root_config_if_vlan;
}

static void cmd_no_int_vlan(FILE *out, char **argv) {
	fprintf(out, "FIXME\n");
}

static void cmd_macstatic(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	int status;
	unsigned char mac[ETH_ALEN];

	ioctl_arg.cmd = SWCFG_MACSTATIC;
	if(!strcmp(argv[0], "no")) {
		ioctl_arg.cmd = SWCFG_DELMACSTATIC;
		argv++;
	}
	status = parse_mac(argv[0], mac);
	assert(!status);
	ioctl_arg.ext.mac = mac;
	ioctl_arg.vlan = parse_vlan(argv[1]);
	ioctl_arg.if_name = if_name_eth(argv[2]);
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if(ioctl_arg.cmd == SWCFG_DELMACSTATIC && status == -1) {
		fprintf(out, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(out);
	}
}

static void __setenpw(FILE *out, int lev, char **argv) {
	int type = 0, status;
	char *secret;
	regex_t regex;
	regmatch_t result;
	
	if(argv[0] == NULL)
		return;
	if(argv[1] != NULL) {
		type = atoi(*argv);
		argv++;
	}
	if(type) {
		/* encrypted password; need to check syntax */
		status = regcomp(&regex, "^\\$1\\$[a-zA-Z0-9\\./]{4}\\$[a-zA-Z0-9\\./]{22}$", REG_EXTENDED);
		assert(!status);
		if(regexec(&regex, *argv, 1, &result, 0)) {
			fputs("ERROR: The secret you entered is not a valid encrypted secret.\n"
					"To enter an UNENCRYPTED secret, do not specify type 5 encryption.\n"
					"When you properly enter an UNENCRYPTED secret, it will be encrypted.\n\n",
					out);
			return;
		}
		secret = *argv;
	} else {
		/* unencrypted password; need to crypt() */
		char salt[] = "$1$....$\0";
		int i;

		srandom(time(NULL) & 0xfffffffL);
		for(i = 3; i < 7; i++)
			salt[i] = salt_base[random() % 64];
		secret = crypt(*argv, salt);
	}
	cfg_lock();
	strcpy(cfg->enable_secret[lev], secret);
	cfg_unlock();
}

static void cmd_setenpw(FILE *out, char **argv) {
	__setenpw(out, CLI_MAX_ENABLE, argv);
}

static void cmd_setenpwlev(FILE *out, char **argv) {
	int lev = atoi(*argv);
	__setenpw(out, lev, argv + 1);
}

static void cmd_noensecret(FILE *out, char **argv) {
	cfg_lock();
	cfg->enable_secret[CLI_MAX_ENABLE][0] = '\0';
	cfg_unlock();
}

static void cmd_noensecret_lev(FILE *out, char **argv) {
	int lev = atoi(argv[1]);
	cfg_lock();
	cfg->enable_secret[lev][0] = '\0';
	cfg_unlock();
}

int valid_host(char *arg, char lookahead) {
	return 1;
}

int valid_no(char *arg, char lookahead) {
	return !strcmp(arg, "no");
}

int valid_0(char *arg, char lookahead) {
	return !strcmp(arg, "0");
}

int valid_5(char *arg, char lookahead) {
	return !strcmp(arg, "5");
}

int valid_lin(char *arg, char lookahead) {
	return 1;
}

static sw_command_t sh_no_int_eth[] = {
	{eth_range,				2,	valid_eth,	cmd_no_int_eth,		RUN,		"Ethernet interface number",					NULL},
	{NULL,					1,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_int_eth[] = {
	{eth_range,				2,	valid_eth,	cmd_int_eth,		RUN,		"Ethernet interface number",					NULL},
	{NULL,					1,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no_int_vlan[] = {
	{vlan_range,			2,	valid_vlan,	cmd_no_int_vlan,	RUN,		"Vlan interface number",						NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_int_vlan[] = {
	{vlan_range,			2,	valid_vlan,	cmd_int_vlan,		RUN,		"Vlan interface number",						NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no_int[] = {
	{"ethernet",			2,	NULL,		NULL,				0,			"Ethernet IEEE 802.3",							sh_no_int_eth},
	{"vlan",				2,	NULL,		NULL,				0,			"LMS Vlans",									sh_no_int_vlan},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

sw_command_t sh_conf_int[] = {
	{"ethernet",			2,	NULL,		NULL,				0,			 "Ethernet IEEE 802.3",							sh_int_eth},
	{"vlan",				2,	NULL,		NULL,				0,			 "LMS Vlans",									sh_int_vlan},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_ifp[] = {
	{eth_range,				2,	valid_eth,	cmd_macstatic,		RUN|PTCNT,	"Ethernet interface number",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_if[] = {
	{"ethernet",			2,	NULL,		NULL,				0,			"Ethernet IEEE 802.3",							sh_macstatic_ifp},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_i[] = {
	{"interface",			2,	NULL,		NULL,				0,			"interface",									sh_macstatic_if},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_vp[] = {
	{vlan_range,			2,	valid_vlan,	NULL,				PTCNT,		"VLAN id of mac address table",					sh_macstatic_i},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_v[] = {
	{"vlan",				2,	NULL,		NULL,				0,			"VLAN keyword",									sh_macstatic_vp},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic[] = {
	{"H.H.H",				2,	valid_mac,	NULL,				PTCNT,		"48 bit mac address",							sh_macstatic_v},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_nomac[] = {
	{"aging-time",			2,	NULL,		NULL,				0,			"Set MAC address table entry maximum age",		NULL},
	{"static",				2,	NULL,		NULL,				0,			"static keyword",								sh_macstatic},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_mac[] = {
	{"aging-time",			2,	NULL,		NULL,				0,			"Set MAC address table entry maximum age",		NULL},
	{"static",				2,	NULL,		NULL,				0,			"static keyword",								sh_macstatic},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_hostname[] = {
	{"WORD",				2,	valid_host,	cmd_hostname,		RUN|PTCNT,	"This system's network name",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_noenlev[] = {
	{priv_range,			1,	valid_priv,	cmd_noensecret_lev,	RUN|PTCNT,	"Level number",									NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_noensecret[] = {
	{"level",				15,	NULL,		NULL,				0,			"Set exec level password",						sh_noenlev},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_noenable[] = {
	{"secret",				15,	NULL,		cmd_noensecret,		RUN,		"Assign the privileged level secret",			sh_noensecret},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no[] = {
	{"enable",				15,	NULL,		NULL,				0,			"Modify enable password parameters",			sh_noenable},
	{"hostname",			2,	NULL,		cmd_nohostname,		RUN,		"Set system's network name",					NULL},
	{"interface",			2,	NULL,		NULL,				0,			"Select an interface to configure",				sh_no_int},
	{"mac-address-table",	2,	NULL,		NULL,				0,			"Configure the MAC address table",				sh_nomac},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_line[] = {
	{"LINE",				15,	valid_lin,	cmd_setenpw,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_lineenc[] = {
	{"LINE",				15,	valid_lin,	cmd_setenpw,		RUN,		"The ENCRYPTED 'enable' secret string",			NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_linelev[] = {
	{"LINE",				15,	valid_lin,	cmd_setenpwlev,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_lineenclev[] = {
	{"LINE",				15,	valid_lin,	cmd_setenpwlev,		RUN,		"The ENCRYPTED 'enable' secret string",			NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_lev_x[] = {
	{"0",					15,	valid_0,	NULL,				PTCNT,		"Specifies an UNENCRYPTED password will follow",sh_secret_linelev},
	{"5",					15,	valid_5,	NULL,				PTCNT,		"Specifies an ENCRYPTED secret will follow",	sh_secret_lineenclev},
	{"LINE",				15,	valid_lin,	cmd_setenpwlev,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_level[] = {
	{priv_range,			1,	valid_priv,	NULL,				PTCNT,		"Level number",									sh_secret_lev_x},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret[] = {
	{"0",					15,	valid_0,	NULL,				PTCNT,		"Specifies an UNENCRYPTED password will follow",sh_secret_line},
	{"5",					15,	valid_5,	NULL,				PTCNT,		"Specifies an ENCRYPTED secret will follow",	sh_secret_lineenc},
//	{"LINE",				15,	valid_lin,	cmd_setenpw,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{"level",				15,	NULL,		NULL,				0,			"Set exec level password",						sh_secret_level},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_enable[] = {
	{"secret",				15,	NULL,		NULL,				0,			"Assign the privileged level secret",			sh_secret},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh[] = {
	{"enable",				2,	NULL,		NULL,				0,			"Modify enable password parameters",			sh_enable},
	{"end",					2,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"exit",				2,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"hostname",			2,	NULL,		NULL,				0,			"Set system's network name",					sh_hostname},
	{"interface",			2,	NULL,		NULL,				0,			"Select an interface to configure",				sh_conf_int},
	{"mac-address-table",	2,	NULL,		NULL,				0,			"Configure the MAC address table",				sh_mac},
	{"no",					2,	valid_no,	NULL,				PTCNT|CMPL, "Negate a command or set its defaults",			sh_no},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			sh};

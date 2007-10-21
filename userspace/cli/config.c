/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <time.h>

#include "command.h"
#include "climain.h"
#include "config_if.h"
#include "config_line.h"
#include "shared.h"
#include "cdp.h"

#include <errno.h>
extern int errno;
char hostname_default[] = "Switch\0";
int vlan_no; /* selected vlan when entering (config-vlan) mode */

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
	struct cdp_ipc_message r;

	ioctl_arg.cmd = SWCFG_ADDIF;
	ioctl_arg.if_name = if_name_eth(arg);
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg)) {
		switch(errno) {
		case ENODEV:
			fprintf(out, "Command rejected: device %s does not exist\n",
					ioctl_arg.if_name);
			return;
		case EBUSY:
			/* Interface already in the switch; just ignore the
			   error and go on */
			break;
		default:
			fprintf(out, "Command rejected: ioctl() failed\n");
			return;
		}
	} else {
		/* Enable CDP on this interface */
		cdp_adm_query(CDP_IPC_IF_ENABLE, ioctl_arg.if_name, &r);
	}

	cmd_root = &command_root_config_if_eth;
	strcpy(sel_eth, ioctl_arg.if_name);
}

/* Add any kind of interface to the switch.
 */
static void cmd_int_any(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	struct cdp_ipc_message r;

	ioctl_arg.cmd = SWCFG_ADDIF;
	ioctl_arg.if_name = argv[0];
	/* Only vlan virtual interfaces must not be added. We do not need
	 * to check this here. Just do the ioctl() and the kernel code
	 * will report EINVAL if we try to add a vif.
	 */
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg)) {
		switch(errno) {
		case ENODEV:
			fprintf(out, "Command rejected: device %s does not exist\n",
					ioctl_arg.if_name);
			return;
		case EINVAL:
			fprintf(out, "I don't like VIFs\n");
			/* FIXME fall back to cmd_int_vlan */
			return;
		case EBUSY:
			/* Interface already in the switch; just ignore the
			   error and go on */
			break;
		default:
			fprintf(out, "Command rejected: ioctl() failed\n");
			return;
		}
	} else {
		/* Enable CDP on this interface */
		cdp_adm_query(CDP_IPC_IF_ENABLE, ioctl_arg.if_name, &r);
	}

	cmd_root = &command_root_config_if_eth;
	strcpy(sel_eth, ioctl_arg.if_name);
}

static void cmd_no_int_eth(FILE *out, char **argv) {
	char *arg = argv[1]; /* argv[0] is "no" */
	struct net_switch_ioctl_arg ioctl_arg;
	struct cdp_ipc_message r;
	
	ioctl_arg.cmd = SWCFG_DELIF;
	ioctl_arg.if_name = if_name_eth(arg);

	/* Disable CDP on this interface */
	cdp_adm_query(CDP_IPC_IF_DISABLE, ioctl_arg.if_name, &r);
	
	/* FIXME nu avem un race aici? codul de kernel pentru socketzi
	 * are nevoie ca device-ul sa fie port in switch => nu poate fi
	 * scos portul pana nu se inchid toti socketzii; aici doar trimit
	 * comanda prin ipc si dureaza pana cdpd inchide socketul.
	 */

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

static void cmd_no_int_any(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	struct cdp_ipc_message r;

	ioctl_arg.cmd = SWCFG_DELIF;
	ioctl_arg.if_name = argv[1]; /* argv[0] is "no" */

	/* Disable CDP on this interface */
	cdp_adm_query(CDP_IPC_IF_DISABLE, ioctl_arg.if_name, &r);

	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
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
	strcpy(CFG->enable_secret[lev], secret);
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
	CFG->enable_secret[CLI_MAX_ENABLE][0] = '\0';
	cfg_unlock();
}

static void cmd_noensecret_lev(FILE *out, char **argv) {
	int lev = atoi(argv[1]);
	cfg_lock();
	CFG->enable_secret[lev][0] = '\0';
	cfg_unlock();
}

static void cmd_set_aging(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	int status;
	
	sscanf(*argv, "%d", &ioctl_arg.ext.nsec);
	ioctl_arg.cmd = SWCFG_SETAGETIME;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status)
		perror("Error setting age time");
}

static void cmd_set_noaging(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	int status;

	ioctl_arg.cmd = SWCFG_SETAGETIME;
	ioctl_arg.ext.nsec = SW_DEFAULT_AGE_TIME;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_linevty(FILE *out, char **argv) {
	sprintf(vty_range, "<%s-%s>", argv[0], argv[1]);
	cmd_root = &command_root_config_line;
}

static void cmd_vlan(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	vlan_no = parse_vlan(*argv);
	ioctl_arg.cmd = SWCFG_ADDVLAN;
	ioctl_arg.vlan = vlan_no; 
	ioctl_arg.ext.vlan_desc = default_vlan_name(vlan_no);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	cmd_root = &command_root_config_vlan;
}

static void cmd_namevlan(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	int status;

	ioctl_arg.cmd = SWCFG_RENAMEVLAN;
	ioctl_arg.vlan = vlan_no;
	ioctl_arg.ext.vlan_desc = *argv; 
	
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if(status == -1 && errno == EPERM)
		fprintf(out, "Default VLAN %d may not have its name changed.\n",
				vlan_no);
}

static void cmd_nonamevlan(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;

	ioctl_arg.cmd = SWCFG_RENAMEVLAN;
	ioctl_arg.vlan = vlan_no;
	ioctl_arg.ext.vlan_desc = default_vlan_name(vlan_no);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_novlan(FILE *out, char **argv) {
	struct net_switch_ioctl_arg ioctl_arg;
	int status;

	ioctl_arg.cmd = SWCFG_DELVLAN;
	ioctl_arg.vlan = parse_vlan(argv[1]);
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if(status == -1 && errno == EPERM)
		fprintf(out, "Default VLAN %d may not be deleted.\n", ioctl_arg.vlan);
}

static void cmd_exit(FILE *out, char **argv) {
	cmd_root = &command_root_config;
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

int valid_age(char *arg, char lookahead)  {
	int age = 0;
	
	sscanf(arg, "%d", &age);

	return (age >=10 && age <= 1000000);
}

int valid_vtyno1(char *arg, char lookahead) {
	int no;

	sscanf(arg, "%d", &no);
	if (no < 0 || no > 15)
		return 0;
	sprintf(vty_range, "<%d-%d>", no+1, 15);
	return 1;
}

int valid_vtyno2(char *arg, char lookahead) {
	int min, max, no;

	sscanf(vty_range, "<%d-%d>", &min, &max);
	sscanf(arg, "%d", &no);
	if (no < min || no > max) 
		return 0;
	return 1;
}

int valid_holdtime(char *arg, char lookahead) {
	int ht = atoi(arg);

	return (ht >= 10 && ht <= 255);
}

int valid_timer(char *arg, char lookahead) {
	int t = atoi(arg);

	return (t >= 5 && t <= 254);
}

int valid_any(char *arg, char lookahead) {
	return 1;
}


static sw_command_t sh_no_int_eth[] = {
	{eth_range,				15,	valid_eth,	cmd_no_int_eth,		RUN,		"Ethernet interface number",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_int_eth[] = {
	{eth_range,				15,	valid_eth,	cmd_int_eth,		RUN,		"Ethernet interface number",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no_int_vlan[] = {
	{vlan_range,			15,	valid_vlan,	cmd_no_int_vlan,	RUN,		"Vlan interface number",						NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_int_vlan[] = {
	{vlan_range,			15,	valid_vlan,	cmd_int_vlan,		RUN,		"Vlan interface number",						NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no_int[] = {
	{"ethernet",			15,	NULL,		NULL,				0,			"Ethernet IEEE 802.3",							sh_no_int_eth},
	{"vlan",				15,	NULL,		NULL,				0,			"LMS Vlans",									sh_no_int_vlan},
	{"WORD",				15,	valid_any,	cmd_no_int_any,		RUN,		"Any interface name",							NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

sw_command_t sh_conf_int[] = {
	{"ethernet",			15,	NULL,		NULL,				0,			"Ethernet IEEE 802.3",							sh_int_eth},
	{"vlan",				15,	NULL,		NULL,				0,			"LMS Vlans",									sh_int_vlan},
	{"WORD",				15,	valid_any,	cmd_int_any,		RUN,		"Any interface name",							NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_ifp[] = {
	{eth_range,				15,	valid_eth,	cmd_macstatic,		RUN|PTCNT,	"Ethernet interface number",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_if[] = {
	{"ethernet",			15,	NULL,		NULL,				0,			"Ethernet IEEE 802.3",							sh_macstatic_ifp},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_i[] = {
	{"interface",			15,	NULL,		NULL,				0,			"interface",									sh_macstatic_if},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_vp[] = {
	{vlan_range,			15,	valid_vlan,	NULL,				PTCNT,		"VLAN id of mac address table",					sh_macstatic_i},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic_v[] = {
	{"vlan",				15,	NULL,		NULL,				0,			"VLAN keyword",									sh_macstatic_vp},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macstatic[] = {
	{"H.H.H",				15,	valid_mac,	NULL,				PTCNT,		"48 bit mac address",							sh_macstatic_v},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_nomac[] = {
	{"aging-time",			15,	NULL,		cmd_set_noaging,	RUN,		"Set MAC address table entry maximum age",		NULL},
	{"static",				15,	NULL,		NULL,				0,			"static keyword",								sh_macstatic},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_macaging[] = {
	{"<10-1000000>",		15,	valid_age,	cmd_set_aging,		RUN,		"Maximum age in seconds",		NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};
static sw_command_t sh_mac[] = {
	{"aging-time",			15,	NULL,		NULL,				0,			"Set MAC address table entry maximum age",		sh_macaging},
	{"static",				15,	NULL,		NULL,				0,			"static keyword",								sh_macstatic},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_hostname[] = {
	{"WORD",				15,	valid_host,	cmd_hostname,		RUN|PTCNT,	"This system's network name",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_noenlev[] = {
	{priv_range,			15,	valid_priv,	cmd_noensecret_lev,	RUN|PTCNT,	"Level number",									NULL},
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

static sw_command_t sh_novlan[] = {
	{"WORD",				15,	valid_vlan,	cmd_novlan,			RUN,		"ISL VLAN IDs 1-4094",							NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no_cdp[] = {
	{"advertise-v2",		15,	NULL,		cmd_no_cdp_v2,		RUN,		"CDP sends version-2 advertisements",			NULL},
	{"run",					15,	NULL,		cmd_no_cdp_run,		RUN,		"",												NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_no[] = {
	{"cdp",					15, NULL,		NULL,				0, 			"Global CDP configuration subcommands",			sh_no_cdp},
	{"enable",				15,	NULL,		NULL,				0,			"Modify enable password parameters",			sh_noenable},
	{"hostname",			15,	NULL,		cmd_nohostname,		RUN,		"Set system's network name",					NULL},
	{"interface",			15,	NULL,		NULL,				0,			"Select an interface to configure",				sh_no_int},
	{"mac-address-table",	15,	NULL,		NULL,				0,			"Configure the MAC address table",				sh_nomac},
	{"vlan",				15,	NULL,		NULL,				0,			"Vlan commands",								sh_novlan},
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
	{"0",					15,	valid_0,	NULL,				PTCNT|CMPL,	"Specifies an UNENCRYPTED password will follow",sh_secret_linelev},
	{"5",					15,	valid_5,	NULL,				PTCNT|CMPL,	"Specifies an ENCRYPTED secret will follow",	sh_secret_lineenclev},
	{"LINE",				15,	valid_lin,	cmd_setenpwlev,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret_level[] = {
	{priv_range,			15,	valid_priv,	NULL,				PTCNT,		"Level number",									sh_secret_lev_x},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_secret[] = {
	{"0",					15,	valid_0,	NULL,				PTCNT|CMPL,	"Specifies an UNENCRYPTED password will follow",sh_secret_line},
	{"5",					15,	valid_5,	NULL,				PTCNT|CMPL,	"Specifies an ENCRYPTED secret will follow",	sh_secret_lineenc},
	{"LINE",				15,	valid_lin,	cmd_setenpw,		RUN,		"The UNENCRYPTED (cleartext) 'enable' secret",	NULL},
	{"level",				15,	NULL,		NULL,				0,			"Set exec level password",						sh_secret_level},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_enable[] = {
	{"secret",				15,	NULL,		NULL,				0,			"Assign the privileged level secret",			sh_secret},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_conf_line_vty2[] = {
	{vty_range,			15,	valid_vtyno2,	cmd_linevty,		RUN|PTCNT,	"Last Line number",									NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_conf_line_vty1[] = {
	{"<0-15>",				15,	valid_vtyno1,NULL,				PTCNT,		"First Line number",							sh_conf_line_vty2},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}

};

static sw_command_t sh_conf_line[] = {
	{"vty",					15,	NULL,		NULL,				0,			"Virtual terminal",								sh_conf_line_vty1},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_vlan[] = {
	{"WORD",				15,	valid_vlan,	cmd_vlan,			RUN,		"ISL VLAN IDs 1-4094",							NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_cdp_holdtime[] = {
	{"<10-255>",			15,	valid_holdtime,		cmd_cdp_holdtime,	RUN,	"Length  of time  (in sec) that receiver must keep this packet",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_cdp_timer[] = {
	{"<5-254>",				15,	valid_timer,		cmd_cdp_timer,		RUN,	"Rate at which CDP packets are sent (in  sec)",	NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_cdp[] = {
	{"advertise-v2",		15,	NULL,		cmd_cdp_version,	RUN,		"CDP sends version-2 advertisements",			NULL},
	{"holdtime",			15,	NULL,		NULL,				0,			"Specify the holdtime (in sec) to be sent in packets", sh_cdp_holdtime},
	{"timer",				15,	NULL,		NULL,				0,			"Specify the rate at which CDP packets are sent (in sec)", sh_cdp_timer},
	{"run",					15,	NULL,		cmd_cdp_run,		RUN,		"",												NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

/* main (config) mode commands */
static sw_command_t sh[] = {
	{"cdp",					15,	NULL,		NULL,				0,			"Global CDP configuration subcommands",			sh_cdp},
	{"enable",				15,	NULL,		NULL,				0,			"Modify enable password parameters",			sh_enable},
	{"end",					15,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"exit",				15,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"hostname",			15,	NULL,		NULL,				0,			"Set system's network name",					sh_hostname},
	{"interface",			15,	NULL,		NULL,				0,			"Select an interface to configure",				sh_conf_int},
	{"line",				15,	NULL,		NULL,				0,			"Configure a terminal line",					sh_conf_line},	
	{"mac-address-table",	15,	NULL,		NULL,				0,			"Configure the MAC address table",				sh_mac},
	{"no",					15,	valid_no,	NULL,				PTCNT|CMPL, "Negate a command or set its defaults",			sh_no},
	{"vlan",				15, NULL,		NULL,				0,			"Vlan commands",								sh_vlan},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_name_vlan[] = {
	{"WORD",				15,	valid_regex,cmd_namevlan,		RUN|PTCNT,	"The ascii name for the VLAN",					NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

static sw_command_t sh_vlan_no[] = {
	{"name",				15,	NULL,		cmd_nonamevlan,		RUN,		"Ascii name of the VLAN",						NULL},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

/* (config-vlan) commands */
static sw_command_t sh_cfg_vlan[] = {
	{"exit",				15,	NULL,		cmd_exit,			RUN,		"Apply changes, bump revision number, and exit mode",NULL},
	{"name",				15,	NULL,		NULL,				0,			"Ascii name of the VLAN",						sh_name_vlan},
	{"no",					15,	NULL,		NULL,				0,			"Negate a command or set its defaults",			sh_vlan_no},
	{"vlan",				15, NULL,		NULL,				0,			"Vlan commands",								sh_vlan},
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			sh};
sw_command_root_t command_root_config_vlan = 			{"%s(config-vlan)%c",	sh_cfg_vlan};

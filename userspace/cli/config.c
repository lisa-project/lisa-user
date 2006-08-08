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
#include "config_line.h"
#include "shared.h"
#include "md5.h"
#include <errno.h>
extern int errno;
char hostname_default[] = "Switch\0";
char prio_range[] = "<0-65535>\0";
char hello_time_range[] = "<1-10>\0";
char forward_delay_range[] = "<6-40>\0";
char max_age_range[] = "<4-30>\0";

char max_age_lim[2] = {4, 30};
char forward_delay_lim[2] = {6, 40};
char hello_time_lim[2] = {1, 10};

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


/**************************************************************************************************
* STP functions and commands
***************************************************************************************************/
int valid_prio_range(char *arg, char lookahead) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 0 || no > 65535)
		return 0;
	return 1;
}

int valid_hello_time(char *arg, char lookahead) {
	char no;
	if(sscanf(arg, "%c", &no) != 1)
		return 0;
	if(no < hello_time_lim[0] || no > hello_time_lim[1])
		return 0;
	return 1;
}


int valid_max_age(char *arg, char lookahead) {
  	char no;
	if(sscanf(arg, "%c", &no) != 1)
		return 0;
	if(no < max_age_lim[0] || no > max_age_lim[1])
		return 0;
	return 1;
}

int valid_forward_delay(char *arg, char lookahead) {
  	char no;
	if(sscanf(arg, "%c", &no) != 1)
		return 0;
	if(no < forward_delay_lim[0] || no > forward_delay_lim[1])
		return 0;
	return 1;
}



void cmd_sw_prio(FILE* out, char** argv) {
  unsigned int prio;
  char *arg = *argv;
  struct net_switch_ioctl_arg ioctl_arg;

  sscanf(arg, "%u", &prio);

  ioctl_arg.cmd = SWCFG_STP_SW_PRIO;
  ioctl_arg.if_name = sel_eth;
  ioctl_arg.ext.prio = prio;

  ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static sw_command_t sh_prio[] = {
  {prio_range, 15, valid_prio_range, cmd_sw_prio, RUN, "Priority values", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

void cmd_sw_hello_time(FILE* out, char** argv) {
  char hello_time;
  char *arg = *argv;
  struct net_switch_ioctl_arg ioctl_arg;

  sscanf(arg, "%c", &hello_time);

  ioctl_arg.cmd = SWCFG_STP_HELLO_TIME;
  ioctl_arg.if_name = sel_eth;
  ioctl_arg.ext.hello_time = hello_time;

  ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}


void cmd_sw_max_age(FILE* out, char** argv) {
  char max_age;
  char *arg = *argv;
  struct net_switch_ioctl_arg ioctl_arg;

  sscanf(arg, "%c", &max_age);

  ioctl_arg.cmd = SWCFG_STP_MAX_AGE;
  ioctl_arg.if_name = sel_eth;
  ioctl_arg.ext.max_age = max_age;

  ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}


void cmd_sw_forward_delay(FILE* out, char** argv) {
  char forward_delay;
  char *arg = *argv;
  struct net_switch_ioctl_arg ioctl_arg;

  sscanf(arg, "%c", &forward_delay);

  ioctl_arg.cmd = SWCFG_STP_FORWARD_DELAY;
  ioctl_arg.if_name = sel_eth;
  ioctl_arg.ext.forward_delay = forward_delay;

  ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

void cmd_sw_stp_enable(FILE* out, char** argv) {
  struct net_switch_ioctl_arg ioctl_arg;
  int stat;

  ioctl_arg.cmd = SWCFG_STP_ENABLE;
  
  stat = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);

  if (stat != 0)
    fprintf(out, "STP already enabled\n");
}


void cmd_sw_stp_disable(FILE* out, char** argv) {
  struct net_switch_ioctl_arg ioctl_arg;
  int stat;

  ioctl_arg.cmd = SWCFG_STP_DISABLE;
  
  stat = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);

  if (stat != 0)
    fprintf(out, "STP already disabled\n");
}

static sw_command_t sh_hello_time[] = {
  {hello_time_range, 15, valid_hello_time, cmd_sw_hello_time, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_max_age[] = {
  {max_age_range, 15, valid_max_age, cmd_sw_max_age, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_forward_delay[] = {
  {forward_delay_range, 15, valid_forward_delay, cmd_sw_forward_delay, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_sw_stp[] = {
  {"enable", 15, NULL, cmd_sw_stp_enable, RUN, "Enable stp", NULL},
  {"disable", 15, NULL, cmd_sw_stp_disable, RUN, "Disable stp", NULL},
  {"priority", 15, NULL, NULL, 0, "Configure switch priority", sh_prio},
  {"hello time", 15, NULL, NULL, 0, "Configure switch hello time", sh_hello_time},
  {"max age", 15, NULL, NULL, 0, "Configure switch max age", sh_max_age},
  {"forward delay", 15, NULL, NULL, 0, "Configure switch forward delay", sh_forward_delay},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};


/**************************************************************************************************
* VTP definitions, functions and commands
***************************************************************************************************/
void set_timestamp(char* timestamp)
{
	time_t rawtime;
	struct tm * timeinfo;
	int ret;
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	memset(timestamp, 0, VTP_TIMESTAMP_SIZE+1);
		
	ret = snprintf(timestamp, VTP_TIMESTAMP_SIZE+1, "%02d%02d%02d%02d%02d%02d", 
		timeinfo->tm_year % 100,
		timeinfo->tm_mon,
		timeinfo->tm_mday,
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec);
}

int valid_vtp_domain(char *arg, char lookahead) 
{
	if(arg == NULL)
		return 0;
	if(strlen(arg) < 1)
		return 0;
	if(strlen(arg) > 32)
		return 0;
	
	return 1;
}

void cmd_sw_vtp_domain(FILE* out, char** argv) 
{
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	
	printf("cmd_sw_vtp_domain: %s %d\n", arg, strlen(arg));

	ioctl_arg.cmd = SWCFG_VTP_SET_DOMAIN;
	ioctl_arg.ext.vtp_domain = arg;
	set_timestamp(ioctl_arg.timestamp);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

int valid_vtp_mode(char *arg, char lookahead) 
{
	if(arg == NULL)
		return 0;
	
	if(strcmp(arg, "client") == 0)
		return 1;
	if(strcmp(arg, "server") == 0)
		return 1;
	if(strcmp(arg, "transparent") == 0)
		return 1;
	
	return 0;
}

char determine_vtp_mode(char* mode)
{
	if(mode == NULL)
		return VTP_MODE_TRANSPARENT;
	
	if(strcmp(mode, "client") == 0)
		return VTP_MODE_CLIENT;
	if(strcmp(mode, "server") == 0)
		return VTP_MODE_SERVER;
	
	return VTP_MODE_TRANSPARENT;
}

void cmd_sw_vtp_mode(FILE* out, char** argv) 
{
	char *mode = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	
	ioctl_arg.cmd = SWCFG_VTP_SET_MODE;	
	ioctl_arg.ext.vtp_mode = determine_vtp_mode(mode);
	set_timestamp(ioctl_arg.timestamp);

	printf("vtp_mode: mode = %d timestamp = %s \n", 
		ioctl_arg.ext.vtp_mode, ioctl_arg.timestamp);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

int valid_vtp_password(char *arg, char lookahead) 
{
	if(arg == NULL)
		return 0;
	if(strlen(arg) < 8)
		return 0;
	if(strlen(arg) > 64)
		return 0;
	
	return 1;
}

unsigned char* get_md5(char* arg)
{
  char* md5_digest = md5(arg);
  print(md5_digest, 16);
  return md5_digest;
}

void cmd_sw_vtp_password(FILE* out, char** argv) 
{
	char *arg = *argv;
	struct net_switch_ioctl_arg ioctl_arg;
	
	ioctl_arg.cmd = SWCFG_VTP_SET_PASSWORD;
	ioctl_arg.ext.vtp_password.password = arg;
	ioctl_arg.ext.vtp_password.md5 = get_md5(arg);
	set_timestamp(ioctl_arg.timestamp);

	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

void cmd_sw_vtp_pruning(FILE* out, char** argv) 
{
	struct net_switch_ioctl_arg ioctl_arg;
	
	ioctl_arg.cmd = SWCFG_VTP_ENABLE_PRUNING;
	set_timestamp(ioctl_arg.timestamp);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

int valid_vtp_version(char *arg, char lookahead) 
{
	char mode;

	mode = atoi(arg);
	
	printf("version = %d\n", mode);	
	if((mode != 1) && (mode != 2))
		return 0;
	
	return 1;
}

void cmd_sw_vtp_version(FILE* out, char** argv) 
{
	char *arg = *argv;
	unsigned char version;
	struct net_switch_ioctl_arg ioctl_arg;

	version = atoi(arg);

	ioctl_arg.cmd = SWCFG_VTP_SET_VERSION;
	ioctl_arg.ext.vtp_version = version;
	set_timestamp(ioctl_arg.timestamp);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static sw_command_t sh_vtp_domain[] = {
  {"name", 15, valid_vtp_domain, cmd_sw_vtp_domain, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_vtp_mode[] = {
  {"[ server | client | transparent ]", 15, valid_vtp_mode, cmd_sw_vtp_mode, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_vtp_password[] = {
  {"password", 15, valid_vtp_password, cmd_sw_vtp_password, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

static sw_command_t sh_vtp_version[] = {
  {"<1-2>", 15, valid_vtp_version, cmd_sw_vtp_version, RUN, "", NULL},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};


static sw_command_t sh_sw_vtp[] = {
  {"domain", 15, NULL, NULL, 0, "Set the name of the VTP administrative domain.", sh_vtp_domain},
  {"file", 15, NULL, NULL, 0, "Configure IFS filesystem file where VTP configuration is stored", NULL},
  {"interface", 15, NULL, NULL, 0, "Configure interface as the preferred source for the VTP IP updater address", NULL},
  {"mode", 15, NULL, NULL, 0, "Configure VTP device mode.", sh_vtp_mode},
  {"password", 15, NULL, NULL, 0, "Set the password for the VTP administrative domain.", sh_vtp_password},
  {"pruning", 15, NULL, cmd_sw_vtp_pruning, RUN, "Set the administrative domain to permit pruning.", NULL},
  {"version", 15, NULL, NULL, 0, "Set the administrative domain to VTP version.", sh_vtp_version},
  {NULL, 0, NULL, NULL, 0, NULL, NULL}
};

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
	{NULL,					0,	NULL,		NULL,				0,			NULL,											NULL}
};

sw_command_t sh_conf_int[] = {
	{"ethernet",			15,	NULL,		NULL,				0,			 "Ethernet IEEE 802.3",							sh_int_eth},
	{"vlan",				15,	NULL,		NULL,				0,			 "LMS Vlans",									sh_int_vlan},
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

static sw_command_t sh_no[] = {
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

/* main (config) mode commands */
static sw_command_t sh[] = {
	{"enable",				15,	NULL,		NULL,				0,			"Modify enable password parameters",			sh_enable},
	{"end",					15,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"exit",				15,	NULL,		cmd_end,			RUN,		"Exit from configure mode",						NULL},
	{"hostname",			15,	NULL,		NULL,				0,			"Set system's network name",					sh_hostname},
	{"interface",			15,	NULL,		NULL,				0,			"Select an interface to configure",				sh_conf_int},
	{"line",				15,	NULL,		NULL,				0,			"Configure a terminal line",					sh_conf_line},	
	{"mac-address-table",	15,	NULL,		NULL,				0,			"Configure the MAC address table",				sh_mac},
	{"no",					15,	valid_no,	NULL,				PTCNT|CMPL, "Negate a command or set its defaults",			sh_no},
	{"vlan",				15, NULL,		NULL,				0,			"Vlan commands",								sh_vlan},
	{"spanning_tree", 15, NULL, NULL, 0, "Configure spanning tree parameters", sh_sw_stp},
	{"vtp", 15, NULL, NULL, 0, "Configure VTP parameters", sh_sw_vtp},
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

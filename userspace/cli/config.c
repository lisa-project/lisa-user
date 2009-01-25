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
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_ADDIF;
	ioctl_arg.ifindex = if_name_eth(arg);
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg)) {
		switch(errno) {
		case ENODEV:
			fprintf(out, "Command rejected: device %s does not exist\n",
					ioctl_arg.ifindex);
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
		cdp_adm_query(CDP_IF_ENABLE, ioctl_arg.ifindex);
	}

	cmd_root = &command_root_config_if_eth;
	strcpy(sel_eth, ioctl_arg.ifindex);
}

/* Add any kind of interface to the switch.
 */
static void cmd_int_any(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_ADDIF;
	ioctl_arg.ifindex = argv[0];
	/* Only vlan virtual interfaces must not be added. We do not need
	 * to check this here. Just do the ioctl() and the kernel code
	 * will report EINVAL if we try to add a vif.
	 */
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg)) {
		switch(errno) {
		case ENODEV:
			fprintf(out, "Command rejected: device %s does not exist\n",
					ioctl_arg.ifindex);
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
		cdp_adm_query(CDP_IF_ENABLE, ioctl_arg.ifindex);
	}

	cmd_root = &command_root_config_if_eth;
	strcpy(sel_eth, ioctl_arg.ifindex);
}

static void cmd_no_int_eth(FILE *out, char **argv) {
	char *arg = argv[1]; /* argv[0] is "no" */
	struct swcfgreq ioctl_arg;
	
	ioctl_arg.cmd = SWCFG_DELIF;
	ioctl_arg.ifindex = if_name_eth(arg);

	/* Disable CDP on this interface */
	cdp_adm_query(CDP_IF_DISABLE, ioctl_arg.ifindex);
	
	/* FIXME nu avem un race aici? codul de kernel pentru socketzi
	 * are nevoie ca device-ul sa fie port in switch => nu poate fi
	 * scos portul pana nu se inchid toti socketzii; aici doar trimit
	 * comanda prin ipc si dureaza pana cdpd inchide socketul.
	 */

	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_int_vlan(FILE *out, char **argv) {
	char *arg = *argv;
	struct swcfgreq ioctl_arg;
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
	struct swcfgreq ioctl_arg;
	int ret;

	ioctl_arg.cmd = SWCFG_DELIF;
	ioctl_arg.ifindex = argv[1]; /* argv[0] is "no" */

	/* Disable CDP on this interface */
	cdp_adm_query(CDP_IF_DISABLE, ioctl_arg.ifindex);
	
	/* FIXME FIXME FIXME FIXME
	 * daca ioctl() de scos interfata esueaza din varii motive,
	 * eu am apucat deja sa opresc cdp-ul pe ea. not good.
	 * pe de alta parte, vezi si FIXME din cmd_no_int_eth
	 */

	if ((ret = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg)))
		perror("ioctl");

	cfg_set_if_tag(argv[1], NULL, NULL);
}

static void cmd_macstatic(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
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
	ioctl_arg.ifindex = if_name_eth(argv[2]);
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
	struct swcfgreq ioctl_arg;
	int status;
	
	sscanf(*argv, "%d", &ioctl_arg.ext.nsec);
	ioctl_arg.cmd = SWCFG_SETAGETIME;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status)
		perror("Error setting age time");
}

static void cmd_set_noaging(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
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
	struct swcfgreq ioctl_arg;

	vlan_no = parse_vlan(*argv);
	ioctl_arg.cmd = SWCFG_ADDVLAN;
	ioctl_arg.vlan = vlan_no; 
	ioctl_arg.ext.vlan_desc = default_vlan_name(vlan_no);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	cmd_root = &command_root_config_vlan;
}

static void cmd_namevlan(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
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
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_RENAMEVLAN;
	ioctl_arg.vlan = vlan_no;
	ioctl_arg.ext.vlan_desc = default_vlan_name(vlan_no);
	
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_novlan(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
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

int valid_any_word(char *arg, char lookahead) {
	return !whitespace(lookahead);
}


static sw_command_t sh_no_int_eth[] = {
	{
		.name   = eth_range,
		.priv   = 15,
		.valid  = valid_eth,
		.func   = cmd_no_int_eth,
		.state  = RUN,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_int_eth[] = {
	{
		.name   = eth_range,
		.priv   = 15,
		.valid  = valid_eth,
		.func   = cmd_int_eth,
		.state  = RUN,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_int_vlan[] = {
	{
		.name   = vlan_range,
		.priv   = 15,
		.valid  = valid_vlan,
		.func   = cmd_no_int_vlan,
		.state  = RUN,
		.doc    = "Vlan interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_int_vlan[] = {
	{
		.name   = vlan_range,
		.priv   = 15,
		.valid  = valid_vlan,
		.func   = cmd_int_vlan,
		.state  = RUN,
		.doc    = "Vlan interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_int[] = {
	{
		.name   = "ethernet",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_no_int_eth
	},
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "LMS Vlans",
		.subcmd = sh_no_int_vlan
	},
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_any_word,
		.func   = cmd_no_int_any,
		.state  = RUN,
		.doc    = "Any interface name",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

sw_command_t sh_conf_int[] = {
	{
		.name   = "ethernet",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_int_eth
	},
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "LMS Vlans",
		.subcmd = sh_int_vlan
	},
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_any_word,
		.func   = cmd_int_any,
		.state  = RUN,
		.doc    = "Any interface name",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic_ifp[] = {
	{
		.name   = eth_range,
		.priv   = 15,
		.valid  = valid_eth,
		.func   = cmd_macstatic,
		.state  = RUN|PTCNT,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic_if[] = {
	{
		.name   = "ethernet",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_macstatic_ifp
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic_i[] = {
	{
		.name   = "interface",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "interface",
		.subcmd = sh_macstatic_if
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic_vp[] = {
	{
		.name   = vlan_range,
		.priv   = 15,
		.valid  = valid_vlan,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "VLAN id of mac address table",
		.subcmd = sh_macstatic_i
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic_v[] = {
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "VLAN keyword",
		.subcmd = sh_macstatic_vp
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macstatic[] = {
	{
		.name   = "H.H.H",
		.priv   = 15,
		.valid  = valid_mac,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "48 bit mac address",
		.subcmd = sh_macstatic_v
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_nomac[] = {
	{
		.name   = "aging-time",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_set_noaging,
		.state  = RUN,
		.doc    = "Set MAC address table entry maximum age",
		.subcmd = NULL
	},
	{
		.name   = "static",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "static keyword",
		.subcmd = sh_macstatic
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_macaging[] = {
	{
		.name   = "<10-1000000>",
		.priv   = 15,
		.valid  = valid_age,
		.func   = cmd_set_aging,
		.state  = RUN,
		.doc    = "Maximum age in seconds",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};
static sw_command_t sh_mac[] = {
	{
		.name   = "aging-time",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set MAC address table entry maximum age",
		.subcmd = sh_macaging
	},
	{
		.name   = "static",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "static keyword",
		.subcmd = sh_macstatic
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_hostname[] = {
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_host,
		.func   = cmd_hostname,
		.state  = RUN|PTCNT,
		.doc    = "This system's network name",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noenlev[] = {
	{
		.name   = priv_range,
		.priv   = 15,
		.valid  = valid_priv,
		.func   = cmd_noensecret_lev,
		.state  = RUN|PTCNT,
		.doc    = "Level number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noensecret[] = {
	{
		.name   = "level",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set exec level password",
		.subcmd = sh_noenlev
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noenable[] = {
	{
		.name   = "secret",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_noensecret,
		.state  = RUN,
		.doc    = "Assign the privileged level secret",
		.subcmd = sh_noensecret
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_novlan[] = {
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_vlan,
		.func   = cmd_novlan,
		.state  = RUN,
		.doc    = "ISL VLAN IDs 1-4094",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_cdp[] = {
	{
		.name   = "advertise-v2",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_no_cdp_v2,
		.state  = RUN,
		.doc    = "CDP sends version-2 advertisements",
		.subcmd = NULL
	},
	{
		.name   = "run",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_no_cdp_run,
		.state  = RUN,
		.doc    = "",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no[] = {
	{
		.name   = "cdp",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Global CDP configuration subcommands",
		.subcmd = sh_no_cdp
	},
	{
		.name   = "enable",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Modify enable password parameters",
		.subcmd = sh_noenable
	},
	{
		.name   = "hostname",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_nohostname,
		.state  = RUN,
		.doc    = "Set system's network name",
		.subcmd = NULL
	},
	{
		.name   = "interface",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Select an interface to configure",
		.subcmd = sh_no_int
	},
	{
		.name   = "mac-address-table",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Configure the MAC address table",
		.subcmd = sh_nomac
	},
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Vlan commands",
		.subcmd = sh_novlan
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_line[] = {
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpw,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) 'enable' secret",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_lineenc[] = {
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpw,
		.state  = RUN,
		.doc    = "The ENCRYPTED 'enable' secret string",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_linelev[] = {
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpwlev,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) 'enable' secret",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_lineenclev[] = {
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpwlev,
		.state  = RUN,
		.doc    = "The ENCRYPTED 'enable' secret string",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_lev_x[] = {
	{
		.name   = "0",
		.priv   = 15,
		.valid  = valid_0,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Specifies an UNENCRYPTED password will follow",
		.subcmd = sh_secret_linelev
	},
	{
		.name   = "5",
		.priv   = 15,
		.valid  = valid_5,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Specifies an ENCRYPTED secret will follow",
		.subcmd = sh_secret_lineenclev
	},
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpwlev,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) 'enable' secret",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret_level[] = {
	{
		.name   = priv_range,
		.priv   = 15,
		.valid  = valid_priv,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "Level number",
		.subcmd = sh_secret_lev_x
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_secret[] = {
	{
		.name   = "0",
		.priv   = 15,
		.valid  = valid_0,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Specifies an UNENCRYPTED password will follow",
		.subcmd = sh_secret_line
	},
	{
		.name   = "5",
		.priv   = 15,
		.valid  = valid_5,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Specifies an ENCRYPTED secret will follow",
		.subcmd = sh_secret_lineenc
	},
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_lin,
		.func   = cmd_setenpw,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) 'enable' secret",
		.subcmd = NULL
	},
	{
		.name   = "level",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set exec level password",
		.subcmd = sh_secret_level
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_enable[] = {
	{
		.name   = "secret",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Assign the privileged level secret",
		.subcmd = sh_secret
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_conf_line_vty2[] = {
	{
		.name   = vty_range,
		.priv   = 15,
		.valid  = valid_vtyno2,
		.func   = cmd_linevty,
		.state  = RUN|PTCNT,
		.doc    = "Last Line number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_conf_line_vty1[] = {
	{
		.name   = "<0-15>",
		.priv   = 15,
		.valid  = valid_vtyno1,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "First Line number",
		.subcmd = sh_conf_line_vty2
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_conf_line[] = {
	{
		.name   = "vty",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Virtual terminal",
		.subcmd = sh_conf_line_vty1
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_vlan[] = {
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_vlan,
		.func   = cmd_vlan,
		.state  = RUN,
		.doc    = "ISL VLAN IDs 1-4094",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_cdp_holdtime[] = {
	{
		.name   = "<10-255>",
		.priv   = 15,
		.valid  = valid_holdtime,
		.func   = cmd_cdp_holdtime,
		.state  = RUN,
		.doc    = "Length  of time  (in sec) that receiver must keep this packet",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_cdp_timer[] = {
	{
		.name   = "<5-254>",
		.priv   = 15,
		.valid  = valid_timer,
		.func   = cmd_cdp_timer,
		.state  = RUN,
		.doc    = "Rate at which CDP packets are sent (in  sec)",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_cdp[] = {
	{
		.name   = "advertise-v2",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_cdp_version,
		.state  = RUN,
		.doc    = "CDP sends version-2 advertisements",
		.subcmd = NULL
	},
	{
		.name   = "holdtime",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Specify the holdtime (in sec) to be sent in packets",
		.subcmd = sh_cdp_holdtime
	},
	{
		.name   = "timer",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Specify the rate at which CDP packets are sent (in sec)",
		.subcmd = sh_cdp_timer
	},
	{
		.name   = "run",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_cdp_run,
		.state  = RUN,
		.doc    = "",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* main (config) mode commands */
static sw_command_t sh[] = {
	{
		.name   = "cdp",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Global CDP configuration subcommands",
		.subcmd = sh_cdp
	},
	{
		.name   = "enable",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Modify enable password parameters",
		.subcmd = sh_enable
	},
	{
		.name   = "end",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_end,
		.state  = RUN,
		.doc    = "Exit from configure mode",
		.subcmd = NULL
	},
	{
		.name   = "exit",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_end,
		.state  = RUN,
		.doc    = "Exit from configure mode",
		.subcmd = NULL
	},
	{
		.name   = "hostname",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set system's network name",
		.subcmd = sh_hostname
	},
	{
		.name   = "interface",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Select an interface to configure",
		.subcmd = sh_conf_int
	},
	{
		.name   = "line",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Configure a terminal line",
		.subcmd = sh_conf_line
	},
	{
		.name   = "mac-address-table",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Configure the MAC address table",
		.subcmd = sh_mac
	},
	{
		.name   = "no",
		.priv   = 15,
		.valid  = valid_no,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Negate a command or set its defaults",
		.subcmd = sh_no
	},
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Vlan commands",
		.subcmd = sh_vlan
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_name_vlan[] = {
	{
		.name   = "WORD",
		.priv   = 15,
		.valid  = valid_regex,
		.func   = cmd_namevlan,
		.state  = RUN|PTCNT,
		.doc    = "The ascii name for the VLAN",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_vlan_no[] = {
	{
		.name   = "name",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_nonamevlan,
		.state  = RUN,
		.doc    = "Ascii name of the VLAN",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* (config-vlan) commands */
static sw_command_t sh_cfg_vlan[] = {
	{
		.name   = "exit",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Apply changes, bump revision number, and exit mode",
		.subcmd = NULL
	},
	{
		.name   = "name",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ascii name of the VLAN",
		.subcmd = sh_name_vlan
	},
	{
		.name   = "no",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Negate a command or set its defaults",
		.subcmd = sh_vlan_no
	},
	{
		.name   = "vlan",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Vlan commands",
		.subcmd = sh_vlan
	},
	SW_COMMAND_LIST_END
};

sw_command_root_t command_root_config = 				{"%s(config)%c",			sh};
sw_command_root_t command_root_config_vlan = 			{"%s(config-vlan)%c",	sh_cfg_vlan};

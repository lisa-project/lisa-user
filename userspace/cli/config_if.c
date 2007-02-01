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
#include <sys/ioctl.h>

#include "climain.h"
#include "config.h"
#include "config_if.h"
#include "ip.h"
#include "cdp.h"

char sel_eth[IFNAMSIZ];
char sel_vlan[IFNAMSIZ];
int int_type;

static void cmd_exit(FILE *out, char **argv) {
	cmd_root = &command_root_config;
}

static void cmd_end(FILE *out, char **argv) {
	cmd_root = &command_root_main;
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

static int parse_mask(char *, char);

static int has_primary_ip(char *dev) {
	int sockfd;
	struct ifreq ifr; 
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	if (ioctl(sockfd, SIOCGIFADDR, &ifr)) {
		close(sockfd);
		return 0;
	}
	close(sockfd);	
	return 1;
}

static int check_ip_primary(struct list_head *ipl, char *addr, char *mask) {
	int nr, match;
	struct ip_addr_entry *entry, *tmp;

	nr = match = 0;
	list_for_each_entry_safe(entry, tmp, ipl, lh) {
		if (nr == 0) {
			if (!strcmp(entry->inet, addr) && !strcmp(entry->mask, mask))
				match = 1;
		}
		list_del(&entry->lh);
		free(entry);
		nr++;
	}
	return (nr > 1 && match);
}

static void change_primary_ip(char *dev, char *addr, char *netmask) {
	int sockfd;
	struct ifreq ifr;
	struct sockaddr s_addr, s_mask, s_brd;
	int j;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return;
	}
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	s_addr.sa_family = AF_INET;
	inet_pton(AF_INET, addr, &s_addr.sa_data[2]);
	ifr.ifr_addr = s_addr;

	if (ioctl(sockfd, SIOCSIFADDR, &ifr)) {
		perror("SIOCSIFADDR");
		return;
	}
	
	s_mask.sa_family = AF_INET;
	inet_pton(AF_INET, netmask, &s_mask.sa_data[2]);
	ifr.ifr_netmask = s_mask;

	if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
		perror("SIOCSIFNETMASK");
		return;
	}

	s_brd.sa_family = AF_INET;
	for (j=0; j<4; j++) {
		s_brd.sa_data[2+j] = s_addr.sa_data[2+j] | ~s_mask.sa_data[2+j];	
	}
	ifr.ifr_broadaddr = s_brd;

	if (ioctl(sockfd, SIOCSIFBRDADDR, &ifr) < 0) {
		perror("SIOCSIFBRDADDR");
		return;
	}

	close(sockfd);
}

static void cmd_ip(FILE *out, char **argv) {
	char ip[32], *addr, *mask;
	int c, secondary = 0;
	int cmd = RTM_NEWADDR;
	int has_primary = 0;
	struct list_head *ipl;
	
	memset(ip, 0, sizeof(ip));
	if (!strcmp(argv[0], "no")) {
		cmd = RTM_DELADDR;
		argv++;
	}
	addr = *argv++;
	c = sprintf(ip, "%s/", addr);
	mask = *argv++;
	sprintf(ip+c, "%d", parse_mask(mask, ' '));
	has_primary = has_primary_ip(sel_vlan);
	if (*argv && !strcmp(*argv, "secondary")) {
		secondary = 1;
		if (cmd == RTM_NEWADDR && !has_primary)
			return;
	}
	if (cmd == RTM_DELADDR && has_primary) {
		ipl = list_ip_addr(sel_vlan, 0); 	
		if (ipl && check_ip_primary(ipl, addr, mask)) { 
			free(ipl);
			fprintf(out, "Must delete secondary before deleting primary\n");
			return;
		}	
	}
	
	if (cmd == RTM_NEWADDR && has_primary && !secondary) {
		change_primary_ip(sel_vlan, addr, mask);
		return;
	}
			
	change_ip_address(cmd, sel_vlan, ip, secondary);
}

static void cmd_no_ip(FILE *out, char **argv) {
	list_ip_addr(sel_vlan, 1);
}

int get_ip_bytes(int *data, char *arg, char lookahead) {
	char *p;
	int valid, i;

	if (arg[0] > '9' || arg[0] < '0')
		return 0;
	for (p=arg,valid=1,i=0; *p; p++) {
		if (*p <= '9' && *p >= '0') {
			data[i] = 10 * data[i] + (*p - '0');
			continue;
		}	
		if (*p == '.' && ++i <= 3)
			continue;
		valid = 0;
		break;
	}
	if ((i < 3 && whitespace(lookahead)) ||
			(arg[strlen(arg)-1] == '.' && whitespace(lookahead)))
		valid = 0;
	return valid;
}

static int valid_ip(char *arg, char lookahead) {
	int addr[4];
	int i;
	
	memset(addr, 0, sizeof(addr));
	if (!get_ip_bytes(addr, arg, lookahead))
		return 0;
	for (i=0; i<4; i++)
		if (addr[i] > 255 || addr[i] < 0)
			return 0;
	return 1;
}

static int valid_mask(char *arg, char lookahead) {
	int addr[4];
	int b, b0 = 0, c=0;
	int i, j;
	
	memset(addr, 0, sizeof(addr));
	if (!get_ip_bytes(addr, arg, lookahead))
		return 0;
	for (i=3; i >=0 ; i--) {
		if (addr[i] > 255 || (addr[i]!=0 && addr[i] <= 127))
			return 0;
		b = addr[i];
		for (j=0; j<8; b>>=1, j++) {
			if ((b&1)^b0) {
				c++;	
				b0=1;
			}
			if (c > 1)
				return 0;
		}
	}
	return 1;
}

static int parse_mask(char *arg, char lookahead) {
	int addr[4];
	int b, b0 = 0, c=0;
	int i, j, nz = 0;
	
	memset(addr, 0, sizeof(addr));
	if (!get_ip_bytes(addr, arg, lookahead))
		return -1;
	for (i=3; i >=0 ; i--) {
		if (addr[i] > 255 || (addr[i]!=0 && addr[i] <= 127))
			return -1;
		b = addr[i];
		for (j=0; j<8; b>>=1, j++) {
			if ((b&1)^b0) {
				c++;	
				b0=1;
			}
			else if (!b0) nz++;
			if (c > 1)
				return -1;
		}
	}
	return 32 - nz;
}

static int valid_sec(char *arg, char lookahead) {
	return !strcmp(arg, "secondary");
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

static int valid_no(char *argv, char lookahead) {
	return !strcmp(argv, "no");
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

static sw_command_t sh_no_eth_cdp[] = {
	{"enable",				2,	NULL, cmd_cdp_if_disable, RUN,			"Enable CDP on interface",							NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_no[] = {
	{"cdp",					2,	NULL,		NULL,			0,			"CDP interface subcommands",						sh_no_eth_cdp},
	{"description",			2,	NULL,		cmd_noethdesc,	RUN,		"Interface specific description",					NULL},
	{"duplex",				2,	NULL,		cmd_du_auto,	RUN,		"Configure duplex operation.",						NULL},
	{"shutdown",			2,	NULL,		cmd_noshutd,	RUN,		"Shutdown the selected interface",					NULL},
	{"speed",				2,	NULL,		cmd_sp_auto,	RUN,		"Configure speed operation.",						NULL},
	{"switchport",			2,	NULL,		NULL,			0,			"Set switching mode characteristics",				sh_noswitchport},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};


static sw_command_t sh_eth_cdp[] = {
	{"enable",				2,	NULL,	cmd_cdp_if_enable, RUN,			"Enable CDP on interface",							NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_eth[] = {
	{"cdp",					2,	NULL,		NULL,			0,			"CDP interface subcommands",						sh_eth_cdp},
	{"description",			2,	NULL,		NULL,			0,			"Interface specific description",					sh_ethdesc},
	{"duplex",				2,	NULL,		NULL,			0,			"Configure duplex operation.",						sh_duplex},
	{"end"	,				2,	NULL,		cmd_end,		RUN,		"End interface configuration mode",					NULL},
	{"exit",				2,	NULL,		cmd_exit,		RUN,		"Exit from interface configuration mode",			NULL},
	{"help",				2,	NULL,		cmd_help,		RUN,		"Descrption of the interactive help system",		NULL},
	{"interface",			2,	NULL,		NULL,			0,			"Select an interface to configure",					sh_conf_int},
	{"no",					2,	NULL,		NULL,			0,			"Negate a command or set its defaults",				sh_no},
	{"shutdown",			2,	NULL,		cmd_shutd,		RUN,		"Shutdown the selected interface",					NULL},
	{"speed",				2,	NULL,		NULL,			0,			"Configure speed operation.",						sh_speed},
	{"switchport",			2,	NULL,		NULL,			0,			"Set switching mode characteristics",				sh_switchport},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_ip_second[] = {
	{"secondary",			2,	valid_sec,	cmd_ip,			RUN|PTCNT|CMPL,		"Make this IP address a secondary address"},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

static sw_command_t sh_ip_netmask[] = {
	{"A.B.C.D",				2,	valid_mask,	cmd_ip,			RUN|PTCNT,		"IP subnet mask",									sh_ip_second},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

static sw_command_t sh_ip_addr[] = {
	{"A.B.C.D",				2,	valid_ip,	NULL,			PTCNT,			"IP address",										sh_ip_netmask},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

static sw_command_t sh_ip[] = {
	{"address",				2,	NULL,		NULL,			0,		"Set the IP address of an interface",				sh_ip_addr},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

static sw_command_t sh_no_ip[] = {
	{"address",				2,	NULL,		cmd_no_ip,		RUN,		"Set the IP address of an interface",				sh_ip_addr},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

static sw_command_t sh_no_vlan[] = {
	{"shutdown",			2,	NULL,		cmd_noshutd_v,	RUN,		"Shutdown the selected interface",					NULL},
	{"ip",					2,	NULL,		NULL,			0,			"Interface Internet Protocol config commands",		sh_no_ip},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_vlan[] = {
	{"end",					2,	NULL,		cmd_end,		RUN,		"Exit from interface configuration mode",			NULL},
	{"exit",				2,	NULL,		cmd_exit,		RUN,		"End interface configuration mode",					NULL},
	{"help",				2,	NULL,		cmd_help,		RUN,		"Description of the interactive help system",		NULL},
	{"ip",					2,	NULL,		NULL,			0,			"Interface Internet Protocol config commands",		sh_ip},
	{"interface",			2,	NULL,		NULL,			0,			"Select an interface to configure",					sh_conf_int},
	{"no",					2,	valid_no,	NULL,			PTCNT|CMPL,	"Negate a command or set its defaults",				sh_no_vlan},
	{"shutdown",			2,	NULL,		cmd_shutd_v,	RUN,		"Shutdown the selected interface",					NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,											NULL}
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};

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
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.vlan = parse_vlan(arg);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noacc_vlan(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETPORTVLAN;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.vlan = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_shutd_v(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_DISABLEVIF;
	sscanf(sel_vlan, "vlan%d", &ioctl_arg.vlan);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noshutd_v(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_ENABLEVIF;
	sscanf(sel_vlan, "vlan%d", &ioctl_arg.vlan);
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_shutd(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_DISABLEPORT;
	ioctl_arg.ifindex = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noshutd(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_ENABLEPORT;
	ioctl_arg.ifindex = sel_eth;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_access(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETACCESS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.access = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_trunk(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.trunk = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_nomode(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETTRUNK;
	ioctl_arg.ifindex = sel_eth;
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
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_addvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_ADDTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_allvlans(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_excvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];
	int i;

	get_vlan_list;
	for(i = 0; i < SW_VLAN_BMP_NO; i++)
		bmp[i] ^= 0xff;
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_novlans(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	memset(bmp, 0xff, SW_VLAN_BMP_NO);
	ioctl_arg.cmd = SWCFG_SETTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.bmp = bmp;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_remvlans(FILE *out, char **argv) {
	char *arg = *argv;
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];

	get_vlan_list;
	ioctl_arg.cmd = SWCFG_DELTRUNKVLANS;
	ioctl_arg.ifindex = sel_eth;
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
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETIFDESC;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.iface_desc = arg;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_noethdesc(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETIFDESC;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.iface_desc = (char *)"";
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_10(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_10;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_100(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_100;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_sp_auto(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSPEED;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.speed = SW_SPEED_AUTO;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_auto(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_AUTO;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_full(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_FULL;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_du_half(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETDUPLEX;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.duplex = SW_DUPLEX_HALF;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_swport_on(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSWPORT;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.switchport = 1;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

static void cmd_swport_off(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;

	ioctl_arg.cmd = SWCFG_SETSWPORT;
	ioctl_arg.ifindex = sel_eth;
	ioctl_arg.ext.switchport = 0;
	ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
}

char VLAN_IDs_of_the_allowed_VLANs[] =
"VLAN IDs of the allowed VLANs when this port is in trunking mode\0";

static sw_command_t sh_acc_vlan[] = {
	{
		.name   = vlan_range,
		.priv   = 1,
		.valid  = valid_vlan,
		.func   = cmd_acc_vlan,
		.state  = RUN,
		.doc    = "VLAN ID of the VLAN when this port is in access mode",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_access[] = {
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set VLAN when interface is in access mode",
		.subcmd = sh_acc_vlan
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noaccess[] = {
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_noacc_vlan,
		.state  = RUN,
		.doc    = "Set VLAN when interface is in access mode",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_mode[] = {
	{
		.name   = "access",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_access,
		.state  = RUN,
		.doc    = "Set trunking mode to ACCESS unconditionally",
		.subcmd = NULL
	},
	{
		.name   = "trunk",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_trunk,
		.state  = RUN,
		.doc    = "Set trunking mode to TRUNK unconditionally",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_addvlan[] = {
	{
		.name   = "WORD",
		.priv   = 2,
		.valid  = valid_vlst,
		.func   = cmd_addvlans,
		.state  = RUN,
		.doc    = VLAN_IDs_of_the_allowed_VLANs,
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_excvlan[] = {
	{
		.name   = "WORD",
		.priv   = 2,
		.valid  = valid_vlst,
		.func   = cmd_excvlans,
		.state  = RUN,
		.doc    = VLAN_IDs_of_the_allowed_VLANs,
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_remvlan[] = {
	{
		.name   = "WORD",
		.priv   = 2,
		.valid  = valid_vlst,
		.func   = cmd_remvlans,
		.state  = RUN,
		.doc    = VLAN_IDs_of_the_allowed_VLANs,
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_all_vlan[] = {
	{
		.name   = "WORD",
		.priv   = 2,
		.valid  = valid_vlst,
		.func   = cmd_setvlans,
		.state  = RUN,
		.doc    = VLAN_IDs_of_the_allowed_VLANs,
		.subcmd = NULL
	},
	{
		.name   = "add",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "add VLANs to the current list",
		.subcmd = sh_addvlan
	},
	{
		.name   = "all",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_allvlans,
		.state  = RUN,
		.doc    = "all VLANs",
		.subcmd = NULL
	},
	{
		.name   = "except",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "all VLANs except the following",
		.subcmd = sh_excvlan
	},
	{
		.name   = "none",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_novlans,
		.state  = RUN,
		.doc    = "no VLANs",
		.subcmd = NULL
	},
	{
		.name   = "remove",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "remove VLANs from the current list",
		.subcmd = sh_remvlan
	},
	SW_COMMAND_LIST_END
};

static char Set_allowed_VLANs[] =
"Set allowed VLANs when interface is in trunking mode\0";

static sw_command_t sh_allowed[] = {
	{
		.name   = "vlan",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = Set_allowed_VLANs,
		.subcmd = sh_all_vlan
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noallowed[] = {
	{
		.name   = "vlan",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_allvlans,
		.state  = RUN,
		.doc    = Set_allowed_VLANs,
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static char Set_allowed_VLAN_characteristics[] =
"Set allowed VLAN characteristics when interface is in trunking mode\0";

static sw_command_t sh_trunk[] = {
	{
		.name   = "allowed",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = Set_allowed_VLAN_characteristics,
		.subcmd = sh_allowed
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_notrunk[] = {
	{
		.name   = "allowed",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = Set_allowed_VLAN_characteristics,
		.subcmd = sh_noallowed
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_switchport[] = {
	{
		.name   = "access",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set access mode characteristics of the interface",
		.subcmd = sh_access
	},
	{
		.name   = "mode",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set trunking mode of the interface",
		.subcmd = sh_mode
	},
	{
		.name   = "trunk",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set trunking characteristics of the interface",
		.subcmd = sh_trunk
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_noswitchport[] = {
	{
		.name   = "access",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set access mode characteristics of the interface",
		.subcmd = sh_noaccess
	},
	{
		.name   = "mode",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_nomode,
		.state  = RUN,
		.doc    = "Set trunking mode of the interface",
		.subcmd = NULL
	},
	{
		.name   = "trunk",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set trunking characteristics of the interface",
		.subcmd = sh_notrunk
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_ethdesc[] = {
	{
		.name   = "LINE",
		.priv   = 2,
		.valid  = valid_desc,
		.func   = cmd_setethdesc,
		.state  = RUN,
		.doc    = "A character string describing this interface",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_speed[] = {
	{
		.name   = "10",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_sp_10,
		.state  = RUN,
		.doc    = "Force 10 Mbps operation",
		.subcmd = NULL
	},
	{
		.name   = "100",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_sp_100,
		.state  = RUN,
		.doc    = "Force 100 Mbps operation",
		.subcmd = NULL
	},
	{
		.name   = "auto",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_sp_auto,
		.state  = RUN,
		.doc    = "Enable AUTO speed configuration",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_duplex[] = {
	{
		.name   = "auto",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_du_auto,
		.state  = RUN,
		.doc    = "Enable AUTO duplex configuration",
		.subcmd = NULL
	},
	{
		.name   = "full",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_du_full,
		.state  = RUN,
		.doc    = "Force full duplex operation",
		.subcmd = NULL
	},
	{
		.name   = "half",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_du_half,
		.state  = RUN,
		.doc    = "Force half-duplex operation",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_eth_cdp[] = {
	{
		.name   = "enable",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_cdp_if_disable,
		.state  = RUN,
		.doc    = "Enable CDP on interface",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no[] = {
	{
		.name   = "cdp",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "CDP interface subcommands",
		.subcmd = sh_no_eth_cdp
	},
	{
		.name   = "description",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_noethdesc,
		.state  = RUN,
		.doc    = "Interface specific description",
		.subcmd = NULL
	},
	{
		.name   = "duplex",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_du_auto,
		.state  = RUN,
		.doc    = "Configure duplex operation.",
		.subcmd = NULL
	},
	{
		.name   = "shutdown",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_noshutd,
		.state  = RUN,
		.doc    = "Shutdown the selected interface",
		.subcmd = NULL
	},
	{
		.name   = "speed",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_sp_auto,
		.state  = RUN,
		.doc    = "Configure speed operation.",
		.subcmd = NULL
	},
	{
		.name   = "switchport",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_swport_off,
		.state  = RUN,
		.doc    = "Set switching mode characteristics",
		.subcmd = sh_noswitchport
	},
	SW_COMMAND_LIST_END
};


static sw_command_t sh_eth_cdp[] = {
	{
		.name   = "enable",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_cdp_if_enable,
		.state  = RUN,
		.doc    = "Enable CDP on interface",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_eth[] = {
	{
		.name   = "cdp",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "CDP interface subcommands",
		.subcmd = sh_eth_cdp
	},
	{
		.name   = "description",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Interface specific description",
		.subcmd = sh_ethdesc
	},
	{
		.name   = "duplex",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Configure duplex operation.",
		.subcmd = sh_duplex
	},
	{
		.name   = "end",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_end,
		.state  = RUN,
		.doc    = "End interface configuration mode",
		.subcmd = NULL
	},
	{
		.name   = "exit",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Exit from interface configuration mode",
		.subcmd = NULL
	},
	{
		.name   = "help",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_help,
		.state  = RUN,
		.doc    = "Descrption of the interactive help system",
		.subcmd = NULL
	},
	{
		.name   = "interface",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Select an interface to configure",
		.subcmd = sh_conf_int
	},
	{
		.name   = "no",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Negate a command or set its defaults",
		.subcmd = sh_no
	},
	{
		.name   = "shutdown",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_shutd,
		.state  = RUN,
		.doc    = "Shutdown the selected interface",
		.subcmd = NULL
	},
	{
		.name   = "speed",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Configure speed operation.",
		.subcmd = sh_speed
	},
	{
		.name   = "switchport",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_swport_on,
		.state  = RUN,
		.doc    = "Set switching mode characteristics",
		.subcmd = sh_switchport
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_ip_second[] = {
	{
		.name   = "secondary",
		.priv   = 2,
		.valid  = valid_sec,
		.func   = cmd_ip,
		.state  = RUN|PTCNT|CMPL,
		.doc    = "Make this IP address a secondary address",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_ip_netmask[] = {
	{
		.name   = "A.B.C.D",
		.priv   = 2,
		.valid  = valid_mask,
		.func   = cmd_ip,
		.state  = RUN|PTCNT,
		.doc    = "IP subnet mask",
		.subcmd = sh_ip_second
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_ip_addr[] = {
	{
		.name   = "A.B.C.D",
		.priv   = 2,
		.valid  = valid_ip,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "IP address",
		.subcmd = sh_ip_netmask
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_ip[] = {
	{
		.name   = "address",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set the IP address of an interface",
		.subcmd = sh_ip_addr
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_ip[] = {
	{
		.name   = "address",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_no_ip,
		.state  = RUN,
		.doc    = "Set the IP address of an interface",
		.subcmd = sh_ip_addr
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_no_vlan[] = {
	{
		.name   = "shutdown",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_noshutd_v,
		.state  = RUN,
		.doc    = "Shutdown the selected interface",
		.subcmd = NULL
	},
	{
		.name   = "ip",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Interface Internet Protocol config commands",
		.subcmd = sh_no_ip
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_vlan[] = {
	{
		.name   = "end",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_end,
		.state  = RUN,
		.doc    = "Exit from interface configuration mode",
		.subcmd = NULL
	},
	{
		.name   = "exit",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "End interface configuration mode",
		.subcmd = NULL
	},
	{
		.name   = "help",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_help,
		.state  = RUN,
		.doc    = "Description of the interactive help system",
		.subcmd = NULL
	},
	{
		.name   = "ip",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Interface Internet Protocol config commands",
		.subcmd = sh_ip
	},
	{
		.name   = "interface",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Select an interface to configure",
		.subcmd = sh_conf_int
	},
	{
		.name   = "no",
		.priv   = 2,
		.valid  = valid_no,
		.func   = NULL,
		.state  = PTCNT|CMPL,
		.doc    = "Negate a command or set its defaults",
		.subcmd = sh_no_vlan
	},
	{
		.name   = "shutdown",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_shutd_v,
		.state  = RUN,
		.doc    = "Shutdown the selected interface",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};

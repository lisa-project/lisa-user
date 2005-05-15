/*
 *    This file is part of Linux Multilayer Switch.
 *
 *    Linux Multilayer Switch is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Linux Multilayer Switch is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Linux Multilayer Switch; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/sockios.h>
#include <linux/net_switch.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <assert.h>

#define INITIAL_BUF_SIZE 4096
#define ETH_ALEN 6

extern void cmd_showmac(FILE *, char *);

unsigned char *parse_hw_addr(char *mac) {
	unsigned char *buf = calloc(sizeof(unsigned char), ETH_ALEN+1);
	unsigned short a0, a1, a2;
	int i;
	
	if (sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
					buf, buf+1, buf+2, buf+3, buf+4, buf+5) == ETH_ALEN) {
		printf("Linux mac format detected: ");		
	}
	else if (sscanf(mac, "%hx.%hx.%hx", &a0, &a1, &a2) == ETH_ALEN/2) {
		printf("Cisco mac format detected: ");
		buf[0] = a0 / 0x100;
		buf[1] = a0 % 0x100;
		buf[2] = a1 / 0x100;
		buf[3] = a1 % 0x100;
		buf[4] = a2 / 0x100;
		buf[5] = a2 % 0x100;
	}
	else {
		printf("Bad mac format makes me very unhappy.\n");
		exit(1);
	}
	for (i=0; i<ETH_ALEN; i++) {
		printf("0x%x ", buf[i]);
	}
	printf("\n");
	return buf;
}

void usage() {
	printf("Usage: swctl [command] [args]\n\n"
		"Command can be any of:\n"
		"  add iface_name\t\t\tAdds an interface to switch.\n"
		"  del iface_name\t\t\tRemoves an iterface from switch\n"
		"  addvlan vlan_no vlan_name\t\tAdds a vlan to the vlan database\n"
		"  delvlan vlan_no\t\t\tDeletes a vlan from the vlan database\n"
		"  chvlan vlan_no new_vlan_name\t\tRenames vlan_no to new_vlan_name\n"
		"  addportvlan iface_name vlan_no\tAdds vlan to allowed vlans of\n"
		"  \t\t\t\t\tinterface (trunk mode)\n"
		"  delportvlan iface_name vlan_no\tRemoves vlan from allowed vlans of\n"
		"  \t\t\t\t\tinterface (trunk mode)\n"
		"  settrunk iface_name flag\t\tPuts interface in trunk (flag=1) or\n"
		"  \t\t\t\t\tnon-trunk (flag=0) mode\n"
		"  setportvlan iface_name vlan_no\tAdd interface in vlan vlan_no\n"
		"  \t\t\t\t\t(non-trunk mode)\n"
		"  clearportmac iface_name\t\tClears fdb entries for interface\n"
		"  setagetime ns\t\t\t\tSets aging interval (in seconds) for fdb entries\n"
		"  macstatic iface_name vlan_no hw_addr\tAdds a static mac to interface in vlan vlan_no\n"
		"  addvif vlan_no\t\t\tCreates a virtual interface for\n"
		"  \t\t\t\t\tgiven vlan\n"
		"  delvif vlan_no\t\t\tRemoves the virtual interface for\n"
		"  \t\t\t\t\tgiven vlan\n"
		"  showmac\t\t\t\tPrints switch forwarding database\n"
		"\n"
	);
}

int main(int argc, char **argv) {
	int sock;
	int status, size;
	struct net_switch_ioctl_arg user_arg;
	char *buf;

	if (argc < 2) {
		usage();
		return 0;
	}
	
	sock = socket(PF_PACKET, SOCK_RAW, 0);
	if (sock == -1) {
		perror("socket");
		return 0;
	}	

	if(!strcmp(argv[1], "add")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_ADDIF;
		user_arg.if_name = argv[2];
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if(status)
			perror("add failed");
		return 0;
	}

	if(!strcmp(argv[1], "del")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_DELIF;
		user_arg.if_name = argv[2];
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if(status)
			perror("del failed");
		return 0;
	}

	if (!strcmp(argv[1], "addvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_ADDVLAN;
		user_arg.vlan = atoi(argv[2]);
		user_arg.ext.vlan_desc = argv[3];
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("addvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "delvlan")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_DELVLAN;
		user_arg.vlan = atoi(argv[2]);
		user_arg.if_name = NULL;
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("delvlan failed");
		return 0;
	}

	if (!strcmp(argv[1], "chvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_RENAMEVLAN;
		user_arg.vlan = atoi(argv[2]);
		user_arg.ext.vlan_desc = argv[3];
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("chvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "addportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_ADDVLANPORT;
		user_arg.if_name = argv[2];
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("addportvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "delportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_DELVLANPORT;
		user_arg.if_name = argv[2];
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("delportvlan failed");
		return 0;
	}
	
	if (!strcmp(argv[1], "settrunk")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_SETTRUNK;
		user_arg.if_name = argv[2];
		user_arg.ext.trunk = atoi(argv[3]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("settrunk failed");
		return 0;	
	}

	if (!strcmp(argv[1], "setportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_SETPORTVLAN;
		user_arg.if_name = argv[2];
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("setportvlan failed");
		return 0;
	}

	if (!strcmp(argv[1], "clearportmac")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_CLEARMACINT;
		user_arg.if_name = argv[2];
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("clearportmac failed");
		return 0;	
	}

	if (!strcmp(argv[1], "setagetime")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_SETAGETIME;
		user_arg.ext.ts.tv_sec = atoi(argv[2]);
		user_arg.ext.ts.tv_nsec = 0;
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("setagetime failed");
		return 0;
	}

	if (!strcmp(argv[1], "macstatic")) {
		if (argc < 5) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_MACSTATIC;
		user_arg.if_name = argv[2];
		user_arg.vlan = atoi(argv[3]);
		user_arg.ext.mac = parse_hw_addr(argv[4]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("macstatic failed");
		return 0;
	}


	if (!strcmp(argv[1], "addvif")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd  = SWCFG_ADDVIF;
		user_arg.vlan = atoi(argv[2]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("addvif failed");
		return 0;
	}

	if (!strcmp(argv[1], "delvif")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.cmd = SWCFG_DELVIF;
		user_arg.vlan = atoi(argv[2]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("delvif failed");
		return 0;
	}

	if (!strcmp(argv[1], "showmac")) {
		buf = (char *)malloc(INITIAL_BUF_SIZE);
		size = INITIAL_BUF_SIZE;
		assert(buf);
		user_arg.if_name = NULL;
		user_arg.cmd = SWCFG_GETMAC;
		memset(&user_arg.ext.marg.addr, 0, ETH_ALEN);
		user_arg.ext.marg.addr_type = SW_FDB_ANY;
		user_arg.vlan = 0;

		do {
			user_arg.ext.marg.buf_size = size;
			user_arg.ext.marg.buf = buf;
			status = ioctl(sock, SIOCSWCFG, &user_arg);
			printf("status %d\n", status);
			if (status == -1) {
				perror("ioctl");
				break;
			}
			if (status == SW_INSUFFICIENT_SPACE) {
				printf("Insufficient buffer space. Realloc'ing ... \n");
				buf = realloc(buf, size+INITIAL_BUF_SIZE);
				assert(buf);
				size += INITIAL_BUF_SIZE;
			}	
			else {
				user_arg.ext.marg.actual_size = status;
				cmd_showmac(stdout, (char *)&user_arg);
				break;
			}	
		} while (status);
		
		return 0;
	}

	/* first command line arg invalid ... */
	usage();

	return 0;
}

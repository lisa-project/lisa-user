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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <assert.h>

#ifdef Lisa
#include "lisa.h"
#endif

#ifdef Linux
#include "linux.h"
#endif

#include "swsock.h"
#include "switch.h"
#include "util.h"
#include "if_generic.h"

#define INITIAL_BUF_SIZE 4096
#define ETH_ALEN 6

/* Dash stuff used for pretty printing of tables */
static const char *dash =
"---------------------------------------"
"---------------------------------------";
/* Dash length must match the number of dashes in the string above */
#define DASH_LENGTH 78
/* Get a null-terminated string (char *) of x dashes */
#define DASHES(x) (dash + DASH_LENGTH - (x))

#define is_digit(arg) ((arg) >= '0' && (arg) <= '9')

int parse_vlan_list(char *list, unsigned char *bmp)
{
	int state = 0;
	int min = 0, max;
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

void parse_hw_addr(char *mac, unsigned char *buf) {
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
}

void usage(void) {
	printf("Usage: swctl [command] [args]\n\n"
		"Command can be any of:\n"
		"  add iface_name\t\t\tAdds an interface to switch.\n"
		"  addtagged iface_name tag_name\t\tAdds an interface to switch and assigns tag.\n"
		"  del [-s] iface_name\t\t\tRemoves an iterface from switch\n"
		"  addvlan vlan_no\t\t\tAdds a vlan to the vlan database\n"
		"  delvlan vlan_no\t\t\tDeletes a vlan from the vlan database\n"
		"  chvlan vlan_no new_vlan_name\t\tRenames vlan_no to new_vlan_name\n"
		"  addportvlan iface_name vlan_no\tAdds vlan to allowed vlans of\n"
		"  \t\t\t\t\tinterface (trunk mode)\n"
		"  delportvlan iface_name vlan_no\tRemoves vlan from allowed vlans of\n"
		"  \t\t\t\t\tinterface (trunk mode)\n"
		"  settrunk iface_name flag\t\tPuts interface in trunk (flag=1) or\n"
		"  setaccess iface_name flag\t\tPuts interface in trunk (flag=1) or\n"
		"  \t\t\t\t\tnon-trunk (flag=0) mode\n"
		"  setportvlan iface_name vlan_no\tAdd interface in vlan vlan_no\n"
		"  \t\t\t\t\t(non-trunk mode)\n"
		"  clearportmac iface_name\t\tClears fdb entries for interface\n"
		"  setagetime seconds / getagetime\t\tSets aging interval (in seconds) for fdb entries\n"
		"  macstatic iface_name vlan_no hw_addr\tAdds a static mac to interface in vlan vlan_no\n"
		"  addvif vlan_no\t\t\tCreates a virtual interface for\n"
		"  \t\t\t\t\tgiven vlan\n"
		"  delvif vlan_no\t\t\tRemoves the virtual interface for\n"
		"  \t\t\t\t\tgiven vlan\n"
		"  showmac\t\t\t\tPrints switch forwarding database\n"
		"  getiftype if_name\t\t\tPrints the type of the the interface\n"
		"  getvlanifs vlan_name\t\t\tPrints the interfaces associated with the vlan\n"
		"  getiflist type\t\t\tPrints all the interfaces switch or routed(1), virtual(0)\n"
		"  setifdesc if_name desc\t\tSet description for an interface\n"
		"  getifdesc if_name\t\t\tDisplay description of the given interface\n"
		"  getmrouters vlan\t\t\tDisplay the ports for which igmp is activated\n"
		"  igmpset vlan flag\t\t\tSet igmp, enable (flag = 1), disable (flag = 0)\n"
		"  igmpget \t\t\t\tSet igmp, enable (flag = 1), disable (flag = 0)\n"
		"  setmrouter vlan if_name flag\t\tPuts mrouter (flag = 1) or unsets it (flag = 0)\n"
		"  enable iface_name\t\t\tEnable the interface.\n"
		"  disable iface_name\t\t\tDisable the interface.\n"
		"\n"
	);
}

int main(int argc, char **argv) {
	int sock;
	int status, size;
	struct swcfgreq user_arg;
	char *buf;

	if (argc < 2) {
		usage();
		return 0;
	}

	/* Initialize sw_ops and shared memory */
	status = switch_init();
	assert(!status);
	
	sock = socket(PF_SWITCH, SOCK_RAW, 0);
	if (sock == -1) {
		perror("socket");
		return 0;
	}	

	if(!strcmp(argv[1], "add")) {
		int if_index;
		if (argc < 3) {
			usage();
			return 0;
		}

		if_index = if_get_index(argv[2], sock);
		status = sw_ops->if_add(sw_ops, if_index, 0);
		if(status)
			perror("add failed");
		return 0;
	}

	if(!strcmp(argv[1], "addtagged")) {
		char buf[IFNAMSIZ];
		int if_index, other_if;
		if (argc < 4) {
			usage();
			return 0;
		}

		if_index = if_get_index(argv[2], sock);
		assert(if_index);

		if (switch_set_if_tag(if_index, argv[3], &other_if)) {
			fprintf(stderr, "Tag %s already assigned to %s\n",
					argv[3], if_get_name(other_if, sock, buf));
			return 1;
		}

		status = sw_ops->if_add(sw_ops, if_index, 0);
		if (status) {
			switch_set_if_tag(if_index, NULL, NULL);
			perror("add failed");
			return 1;
		}
		return 0;
	}

	if(!strcmp(argv[1], "del")) {
		int if_index;
		int silent = 0;
		if (argc < 3) {
			usage();
			return 0;
		}

		do {
			if (argc < 4)
				break;
			if (strcmp(argv[3], "-s"))
				break;
			silent = 1;
		} while(0);

		if_index = if_get_index(argv[2], sock);
		assert(if_index);

		status = sw_ops->if_remove(sw_ops, if_index);
		if (status && !silent) {
			perror("del failed");
			return 1;
		}
		/* On silent mode, ignore the ioctl() exit status and delete
		 * the tag anyway. This is useful for Xen integration: Xen
		 * first unregisters vifX.0, and then calls the vif-lisa
		 * script. The netdevice is removed from the switch immediately
		 * (via netdevice notification handler in switch kernel module,
		 * but the interface tag must be deleted later, via userspace
		 * brctl).
		 */
		switch_set_if_tag(if_index, NULL, NULL);
		return 0;
	}

	if (!strcmp(argv[1], "addvlan")) {
		if (argc < 3) {
			usage();
			return 0;
		}

		status = sw_ops->vlan_add(sw_ops, atoi(argv[2]));
		if (status)
			perror("addvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "delvlan")) {
		if (argc < 3) {
			usage();
			return 0;
		}

		status = sw_ops->vlan_del(sw_ops, atoi(argv[2]));
		if (status)
			perror("delvlan failed");
		return 0;
	}

	if (!strcmp(argv[1], "chvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}

		status = switch_set_vlan_desc(atoi(argv[2]), argv[3]);
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
		user_arg.ifindex = if_get_index(argv[2], sock);
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
		user_arg.ifindex = if_get_index(argv[2], sock);
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("delportvlan failed");
		return 0;
	}

	if (!strcmp(argv[1], "settrunkvlans")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		unsigned char vlans[SW_VLAN_BMP_NO];

		parse_vlan_list(argv[3], vlans);

		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_set_trunk_vlans(sw_ops,
				user_arg.ifindex, vlans);

		if (status)
			perror("settrunkvlans failed");
		return 0;

	}

	if (!strcmp(argv[1], "addtrunkvlans")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		unsigned char vlans[SW_VLAN_BMP_NO];

		parse_vlan_list(argv[3], vlans);

		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_add_trunk_vlans(sw_ops,
				user_arg.ifindex, vlans);

		if (status)
			perror("addtrunkvlans failed");
		return 0;

	}

	if (!strcmp(argv[1], "deltrunkvlans")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		unsigned char vlans[SW_VLAN_BMP_NO];

		parse_vlan_list(argv[3], vlans);

		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_del_trunk_vlans(sw_ops,
				user_arg.ifindex, vlans);

		if (status)
			perror("deltrunkvlans failed");
		return 0;

	}

	if (!strcmp(argv[1], "getvdb")) {
		if (argc < 2) {
			usage();
			return 0;
		}
		unsigned char vlans[SW_VLAN_BMP_NO];
		int i;

		status = sw_ops->get_vdb(sw_ops, vlans);

		if (status)
			perror("getvdb failed");

		printf("VDB: ");

		for (i=0 ; i < SW_MAX_VLAN; i++)
		{
			if (vlans[i / 8] & (1 << (i % 8)))
			{
				printf("%d, ",i);
			}
		}
		printf("\n");

		return 0;

	}

	if (!strcmp(argv[1], "ifgetcfg")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		unsigned char vlans[SW_VLAN_BMP_NO];
		int flags, access_vlan, i;

		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_get_cfg(sw_ops,
				user_arg.ifindex, &flags, &access_vlan,
					vlans);

		if (status)
			perror("ifgetcfg failed");

		printf("ALLOWED VLANS: ");

		for (i=0 ; i < SW_MAX_VLAN; i++)
		{
			if (vlans[i / 8] & (1 << (i % 8)))
			{
				printf("%d, ",i);
			}
		}
		printf("\n");

		return 0;

	}

	if (!strcmp(argv[1], "settrunk")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_set_mode(sw_ops,
				user_arg.ifindex, IF_MODE_TRUNK, atoi(argv[3]));

		if (status)
			perror("settrunk failed");
		return 0;	
	}

	if (!strcmp(argv[1], "setaccess")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_set_mode(sw_ops,
				user_arg.ifindex, IF_MODE_ACCESS, atoi(argv[3]));

		if (status)
			perror("settrunk failed");
		return 0;
	}

	if (!strcmp(argv[1], "setportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.ifindex = if_get_index(argv[2], sock);
		user_arg.vlan = atoi(argv[3]);
		status = sw_ops->if_set_port_vlan(sw_ops,
				user_arg.ifindex, user_arg.vlan);
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
		user_arg.ifindex = if_get_index(argv[2], sock);
		status = ioctl(sock, SIOCSWCFG, &user_arg);
		if (status)
			perror("clearportmac failed");
		return 0;	
	}

	if (!strcmp(argv[1], "macstatic")) {
		if (argc < 5) {
			usage();
			return 0;
		}
		user_arg.ifindex = if_get_index(argv[2], sock);
		user_arg.vlan = atoi(argv[3]);
		parse_hw_addr(argv[4], user_arg.ext.mac.addr);
		status = sw_ops->vlan_set_mac_static(sw_ops, user_arg.ifindex,
				user_arg.vlan,user_arg.ext.mac.addr);
		if (status)
			perror("macstatic failed");
		return 0;
	}

	if (!strcmp(argv[1], "delmacstatic")) {
		if (argc < 5) {
			usage();
			return 0;
		}
		user_arg.ifindex = if_get_index(argv[2], sock);
		user_arg.vlan = atoi(argv[3]);
		parse_hw_addr(argv[4], user_arg.ext.mac.addr);
		status = sw_ops->vlan_del_mac_static(sw_ops, user_arg.ifindex,
				user_arg.vlan,user_arg.ext.mac.addr);
		if (status)
			perror("delmacstatic failed");
		return 0;
	}

	if (!strcmp(argv[1], "getmac")) {
		if (argc < 6) {
			usage();
			return 0;
		}

		int status, i;
		int mac_type;
		char if_name[SW_MAX_PORT_DESC];
		struct list_head macs, *iter, *tmp;
		struct net_switch_mac_e *entry;

		mac_type = atoi(argv[4]);
		user_arg.ifindex = if_get_index(argv[2], sock);
		user_arg.vlan = atoi(argv[3]);
		if (strcmp(argv[5], "no_address"))
		parse_hw_addr(argv[5], user_arg.ext.mac.addr);

		switch(mac_type){
		case 0:
			mac_type = SW_FDB_ANY;
			break;
		case 1:
			mac_type = SW_FDB_MAC_ANY;
			break;
		case 2:
			mac_type = SW_FDB_MAC_STATIC;
			break;
		case 3:
			mac_type = SW_FDB_MAC_DYNAMIC;
			break;
		case 4:
			mac_type = SW_FDB_IGMP_ANY;
			break;
		}

		INIT_LIST_HEAD(&macs);

		if (!strcmp(argv[5], "no_address"))
			status = sw_ops->get_mac(sw_ops, user_arg.ifindex,
				user_arg.vlan, mac_type, NULL, &macs);
		else
			status = sw_ops->get_mac(sw_ops, user_arg.ifindex,
				user_arg.vlan, mac_type, user_arg.ext.mac.addr, 				&macs);

		if(status < 0)
			return -1;

		printf("  Vlan  |  Interface | MAC \n");
		printf("%s\n", DASHES(21));
		list_for_each_safe(iter, tmp, &macs) {
			entry = list_entry(iter, struct net_switch_mac_e, lh);
			if_get_name(entry->ifindex, sock, if_name);

			printf("%8d|%12s|", entry->vlan, if_name);

			for (i=0; i< ETH_ALEN; i++) {
				printf("0x%x ", entry->addr[i]);
			}
			printf("\n");

			list_del(iter);
			free(entry);
		}

		return 0;
	}


	if (!strcmp(argv[1], "addvif")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		int interface;

		user_arg.vlan = atoi(argv[2]);
		status = sw_ops->vif_add(sw_ops, user_arg.vlan, &interface);

		if (status)
			perror("addvif failed");
		printf("[vlan] %d\t[vif_index] %d\n",user_arg.vlan, interface);

		return 0;
	}

	if (!strcmp(argv[1], "delvif")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.vlan = atoi(argv[2]);
		status = sw_ops->vif_del(sw_ops, user_arg.vlan);

		if (status)
			perror("delvif failed");
		printf("Succesfully deleted [vif] %d\n", user_arg.vlan);
		return 0;
	}

	if (!strcmp(argv[1], "showmac")) {
		buf = (char *)malloc(INITIAL_BUF_SIZE);
		size = INITIAL_BUF_SIZE;
		assert(buf);
		user_arg.ifindex = 0;
		user_arg.cmd = SWCFG_GETMAC;
		memset(user_arg.ext.mac.addr, 0, ETH_ALEN);
		user_arg.ext.mac.type = SW_FDB_ANY;
		user_arg.vlan = 0;

		do {
			user_arg.buf.size = size;
			user_arg.buf.addr = buf;
			status = ioctl(sock, SIOCSWCFG, &user_arg);
			printf("status %d\n", status);
			if (status == -1) {
				if (errno == ENOMEM) {
					printf("Insufficient buffer space. Realloc'ing ... \n");
					buf = realloc(buf, size+INITIAL_BUF_SIZE);
					assert(buf);
					size += INITIAL_BUF_SIZE;
					continue;
				}
				perror("ioctl");
				return (-1);
			}
		} while (status < 0);
		/* the ioctl return value is actually the result size */
		print_mac(stdout, (void *)&user_arg, status, NULL, NULL);
		
		return 0;
	}

	if (!strcmp(argv[1], "getiftype")) {
		if(argc < 3){
			usage();
			return 0;
		}
		int type, ifvlan;
		user_arg.ifindex = if_get_index(argv[2], sock);
		status = sw_ops->if_get_type(sw_ops,
				user_arg.ifindex, &type, &ifvlan);
		if (status < 0) {
			printf("getting if type failed\n");
			return status;
		}

		switch (type) {
		case IF_TYPE_VIF:
			printf("[%s]\tvlan %d interface\n", argv[2], ifvlan);
			break;
		case IF_TYPE_SWITCHED:
			printf("[%s]\tethernet interface\n", argv[2]);
			break;
		case IF_TYPE_ROUTED:
			printf("[%s]\trouted interface\n", argv[2]);
			break;
		default:
			printf("unknown interface type\n");
		}

		return status;
	}

	if (!strcmp(argv[1], "getvlanifs")) {
		if(argc < 3){
			usage();
			return 0;
		}
		int *interfaces;
		int no_ifs = 0;
		int i;

		interfaces = malloc(sizeof(int) * INITIAL_BUF_SIZE);
		if (!interfaces) {
			perror("Couldn't allocate buffer for interfaces\n");
			return -1;
		}

		user_arg.vlan = atoi(argv[2]);
		status = sw_ops->get_vlan_interfaces(sw_ops, user_arg.vlan,
					interfaces, &no_ifs);

		if (status < 0) {
			printf("getting vlan interfaces failed\n");
			return status;
		}

		printf("vlan\t|\tinterfaces\t\n");
		printf("%s\n", DASHES(30));

		printf("%s\t|",argv[2]);
		for (i = 0; i < no_ifs; ++i) {
			printf("%3d", interfaces[i]);
		}
		printf("\n");
		free(interfaces);
		return status;
	}

	if (!strcmp(argv[1], "getiflist")) {
		int type;
		struct list_head net_devs, *iter, *tmp;
		struct net_switch_device *dev;

		if (argc < 3) {
			usage();
			return 0;
		}

		if (atoi(argv[2]))
			type = SW_IF_SWITCHED | SW_IF_ROUTED;
		else
			type = SW_IF_VIF;

		INIT_LIST_HEAD(&net_devs);
		status = sw_ops->get_if_list(sw_ops, type, &net_devs);
		if (status < 0) {
			printf("Failed to get all interfaces\n");
			return status;
		}

		list_for_each_safe(iter, tmp, &net_devs) {
			dev = list_entry(iter, struct net_switch_device, lh);
			printf("Int%02d ", dev->ifindex);

			list_del(iter);
			free(dev);
		}
		printf("\n");
		return status;
	}

	if (!strcmp(argv[1], "getifdesc")) {
		if (argc < 2) {
			usage();
			return 0;
		}

		char desc[SW_MAX_PORT_DESC];
		int if_index, status;

		if_index = if_get_index(argv[2], sock);
		status = sw_ops->if_get_desc(sw_ops, if_index, desc);
		if(status < 0) {
			perror("Failed to get interface description");
			return status;
		}

		printf("[%2s]\t%s\n", argv[2], desc);

		return status;
	}

	if (!strcmp(argv[1], "setagetime")) {
		int age_time;
		if (argc < 3) {
			usage();
			return 0;
		}
		age_time = atoi(argv[2]);

		status = sw_ops->set_age_time(sw_ops, age_time);
		if (status)
			perror("Failed to set age time");

		return status;
	}

	if (!strcmp(argv[1], "setifdesc")) {
		if (argc < 3) {
			usage();
			return 0;
		}

		int if_index, status;

		if_index = if_get_index(argv[2], sock);
		status = sw_ops->if_set_desc(sw_ops, if_index, argv[3]);

		if(status < 0) {
			perror("Failed to set interface description");
			return status;
		}

		printf("Description set successfully for _ %s _\n", argv[2]);

		return status;
	}

	if (!strcmp(argv[1], "getagetime")) {
		int age_time;

		status = sw_ops->get_age_time(sw_ops, &age_time);
		if (status) {
			perror("Failed to get age time");
			return status;
		}

		printf("Age time %d\n", age_time);
		return status;
	}

	if (!strcmp(argv[1], "getmrouters")) {
		if (argc < 3) {
			usage();
			return 0;
		}

		int status;
		int vlan = atoi(argv[2]);
		char if_name[SW_MAX_PORT_DESC];
		struct list_head mrouters, *iter, *tmp;
		struct net_switch_mrouter_e *entry;

		INIT_LIST_HEAD(&mrouters);
		status = sw_ops->mrouters_get(sw_ops, vlan, &mrouters);
		if(status < 0)
			return -1;

		printf("  Vlan  |  Interface  \n");
		printf("%s\n", DASHES(21));
		list_for_each_safe(iter, tmp, &mrouters) {
			entry = list_entry(iter, struct net_switch_mrouter_e, lh);
			if_get_name(entry->ifindex, sock, if_name);

			printf("%8d|%6s\n", entry->vlan, if_name);

			list_del(iter);
			free(entry);
		}

		return 0;
	}

	if (!strcmp(argv[1], "setmrouter")) {
		if (argc < 5) {
			usage();
			return 0;
		}

		int vlan = atoi(argv[2]);
		int ifindex = if_get_index(argv[3], sock);
		int option = atoi(argv[4]);

		int status = sw_ops->mrouter_set(sw_ops, vlan, ifindex, option);

		if(status < 0)
			return status;
		printf("Mrouter was successfully set\n");

		return 0;
	}

	if (!strcmp(argv[1], "igmpset")) {
		if (argc < 4) {
			usage();
			return 0;
		}

		int vlan = atoi(argv[2]);
		int snoop = atoi(argv[3]);
		int status = sw_ops->igmp_set(sw_ops, vlan, snoop);

		return status;
	}

	if (!strcmp(argv[1], "igmpget")) {
		int snoop;
		int status = sw_ops->igmp_get(sw_ops, NULL, &snoop);
		printf("IGMP is ");
		if (snoop)
			printf("on\n");
		else
			printf("off\n");

		return status;
	}

	if(!strcmp(argv[1], "enable")) {
		int if_index;
		if (argc < 3) {
			usage();
			return 0;
		}

		if_index = if_get_index(argv[2], sock);
		status = sw_ops->if_enable(sw_ops, if_index);
		if(status)
			perror("enable failed");
		return 0;
	}

	if(!strcmp(argv[1], "disable")) {
		int if_index;
		if (argc < 3) {
			usage();
			return 0;
		}

		if_index = if_get_index(argv[2], sock);
		status = sw_ops->if_disable(sw_ops, if_index);
		if(status)
			perror("enable failed");
		return 0;
	}

	/* first command line arg invalid ... */
	usage();

	return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <linux/sockios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <assert.h>

#include "swctl.h"

void usage() {
	printf("Usage: swctl [command] [args]\n\n"
		"Command can be any of:\n"
		"  add interface_name\tAdds an interface to switch.\n"
		"  del interface_name\tRemoves an iterface from switch\n"
		"  addvlan vlan_no vlan_name\tAdds a vlan to the vlan database\n"
		"  delvlan vlan_no\tDeletes a vlan from the vlan database\n"
		"  chvlan vlan_no new_vlan_name\tRenames vlan_no to new_vlan_name\n"
		"  addportvlan interface_name vlan_no\tAdds interface in vlan vlan_no\n"
		"  delportvlan interface_name vlan_no\tRemoves interface from vlan vlan_no\n\n"
	);
}

int main(int argc, char **argv) {
	int sock;
	int status;
	struct sw_vdb_arg user_arg;

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
		status = ioctl(sock, SIOCSWADDIF, argv[2]);
		if(status)
			perror("add failed");
		return 0;
	}

	if(!strcmp(argv[1], "del")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		status = ioctl(sock, SIOCSWDELIF, argv[2]);
		if(status)
			perror("del failed");
		return 0;
	}

	if (!strcmp(argv[1], "addvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.vlan = atoi(argv[2]);
		user_arg.name = strdup(argv[3]);
		status = ioctl(sock, SIOCSWADDVLAN, &user_arg);
		if (status)
			perror("addvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "delvlan")) {
		if (argc < 3) {
			usage();
			return 0;
		}
		user_arg.vlan = atoi(argv[2]);
		user_arg.name = NULL;
		status = ioctl(sock, SIOCSWDELVLAN, &user_arg);
		if (status)
			perror("delvlan failed");
		return 0;
	}

	if (!strcmp(argv[1], "chvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.vlan = atoi(argv[2]);
		user_arg.name = strdup(argv[3]);
		status = ioctl(sock, SIOCSWRENAMEVLAN, &user_arg);
		if (status)
			perror("chvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "addportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.name = strdup(argv[2]);
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWADDVLANPORT, &user_arg);
		if (status)
			perror("addportvlan failed");
		return 0;	
	}

	if (!strcmp(argv[1], "delportvlan")) {
		if (argc < 4) {
			usage();
			return 0;
		}
		user_arg.name = strdup(argv[2]);
		user_arg.vlan = atoi(argv[3]);
		status = ioctl(sock, SIOCSWDELVLANPORT, &user_arg);
		if (status)
			perror("delportvlan failed");
		return 0;
	}

	/* first command line arg invalid ... */
	usage();

	return 0;
}

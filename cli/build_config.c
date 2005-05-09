#include <linux/net_switch.h>
#include <linux/if.h>
#define _SYS_TYPES_H 1

#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
extern int errno;
#include <assert.h>
#include <unistd.h>

#include "climain.h"

int list_vlans_token(unsigned char *bmp, int vlan, char *token) {
	int i, min;
	while(vlan <= SW_MAX_VLAN && sw_forbidden_vlan(bmp, vlan))
		vlan++;
	if(vlan > SW_MAX_VLAN)
		return 0;
	i = sprintf(token, "%d", vlan);
	min = vlan;
	while(vlan <= SW_MAX_VLAN && sw_allowed_vlan(bmp, vlan))
		vlan++;
	if(vlan - min == 1)
		return vlan;
	if(vlan - min > 2)
		sprintf(token + i, "-%d", vlan - 1);
	else
		vlan--;
	return vlan;
}

void list_vlans(unsigned char *bmp, FILE *out) {
	char buf[80];
	char token[11]; /* max len is for ",nnnn-nnnn" */
	int vlan = 1;
	int first = 1;
	int i;

	token[0] = ',';
	i = sprintf(buf, " switchport trunk allowed vlan ");
	while((vlan = list_vlans_token(bmp, vlan, token + 1))) {
		if(i + strlen(token + first) >= sizeof(buf)) {
			fputs(buf, out);
			fputc('\n', out);
			i = sprintf(buf, " switchport trunk allowed vlan add ");
			first = 1;
		}
		i += sprintf(buf + i, token + first);
		first = 0;
	}
	fputs(buf, out);
	fputc('\n', out);
}

int build_int_eth_config(FILE *out, int num) {
	char if_name[IFNAMSIZ];
	struct net_switch_ioctl_arg ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];
	char desc[SW_MAX_PORT_DESC];
	int need_trunk_vlans = 1;

	sprintf(if_name, "eth%d", num);
	ioctl_arg.cmd = SWCFG_GETIFCFG;
	ioctl_arg.if_name = if_name;
	ioctl_arg.ext.cfg.forbidden_vlans = bmp;
	ioctl_arg.ext.cfg.description = desc;
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg) == -1)
		return errno;

	fprintf(out, "!\ninterface ethernet %d\n", num);
	/* description */
	if(strlen(desc))
		fprintf(out, " description %s\n", desc);
	/* switchport access vlan */
	if(ioctl_arg.ext.cfg.access_vlan != 1)
		fprintf(out, " switchport access vlan %d\n",
				ioctl_arg.ext.cfg.access_vlan);
	/* switchport trunk allowed vlan */
	do {
		int i;
		if((bmp[0] | 0x01) != 0x01)
			break;
		if((bmp[SW_VLAN_BMP_NO - 1] | 0xf0) != 0xf0)
			break;
		for(i = SW_VLAN_BMP_NO - 2; i > 0 && !bmp[i]; i--);
		if(i)
			break;
		need_trunk_vlans = 0;
	} while(0);
	if(need_trunk_vlans)
		list_vlans(bmp, out);
	/* switchport mode */
	if(ioctl_arg.ext.cfg.flags & SW_PFL_ACCESS)
		fprintf(out, " switchport mode access\n");
	if(ioctl_arg.ext.cfg.flags & SW_PFL_TRUNK)
		fprintf(out, " switchport mode trunk\n");
	/* speed */
	if(ioctl_arg.ext.cfg.speed != SW_SPEED_AUTO) {
		char *speed = NULL;
		switch(ioctl_arg.ext.cfg.speed) {
		case SW_SPEED_10:
			speed = "10";
			break;
		case SW_SPEED_100:
			speed = "100";
			break;
		case SW_SPEED_1000:
			speed = "1000";
			break;
		}
		fprintf(out, " speed %s\n", speed);
	}
	/* duplex */
	if(ioctl_arg.ext.cfg.duplex != SW_DUPLEX_AUTO) {
		char *duplex = NULL;
		switch(ioctl_arg.ext.cfg.duplex) {
		case SW_DUPLEX_HALF:
			duplex = "half";
			break;
		case SW_DUPLEX_FULL:
			duplex = "full";
			break;
		}
		fprintf(out, " duplex %s\n", duplex);
	}
	return 0;
}

int build_config(FILE *out) {
	char buf[4096], *p1, *p2;
	FILE *f;

	/* hostname */
	gethostname(buf, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0'; /* paranoia :P */
	fprintf(out, "!\nhostname %s\n", buf);
	/* physical interfaces */
	f = fopen("/proc/net/dev", "r");
	assert(f != NULL);
	while(fgets(buf, sizeof(buf), f) != NULL) {
		if((p1 = strchr(buf, ':')) == NULL)
			continue;
		*p1 = '\0';
		for(p1 = buf; *p1 == ' '; p1++);
		if(strstr(p1, "eth") != p1)
			continue;
		p1 += 3;
		for(p2 = p1; is_digit(*p2); p2++);
		if(*p2 != '\0')
			continue;
		build_int_eth_config(out, atoi(p1));
	}

	return 0;
}

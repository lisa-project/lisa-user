#include <linux/net_switch.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
extern int errno;
#include <assert.h>
#include <unistd.h>

#include "climain.h"
#include "shared.h"
#include "ip.h"

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

#define INITIAL_BUF_SIZE 4096
void dump_static_macs(FILE *out) {
	char *buf, *ptr;
	int status, size;
	struct net_switch_ioctl_arg ioctl_arg;

	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	assert(buf);
	ioctl_arg.if_name = NULL;
	ioctl_arg.cmd = SWCFG_GETMAC;
	memset(&ioctl_arg.ext.marg.addr, 0, ETH_ALEN);
	ioctl_arg.ext.marg.addr_type = SW_FDB_STATIC;
	ioctl_arg.vlan = 0;

	do {
		ioctl_arg.ext.marg.buf_size = size;
		ioctl_arg.ext.marg.buf = buf;
		status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
		if (status == -1) {
			if (errno == ENOMEM) {
				buf = realloc(buf, size+INITIAL_BUF_SIZE);
				assert(buf);
				size += INITIAL_BUF_SIZE;
				continue;
			}
			free(buf);
			return;
		}
	} while (status < 0);
	/* status holds sizeof(struct) * count */
	for(ptr = buf; ptr - buf < status; ptr += sizeof(struct net_switch_mac)) {
		struct net_switch_mac *mac = (struct net_switch_mac *)ptr;
		unsigned char *addr = mac->addr;
		int eth_no;

		if(ptr == buf)
			fprintf(out, "!\n");
		sscanf(mac->port, "eth%d", &eth_no);
		fprintf(out, "mac-address-table static "
				"%02hhx%02hhx.%02hhx%02hhx.%02hhx%02hhx "
				"vlan %d interface ethernet %d\n",
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
				mac->vlan, eth_no);
	}
	free(buf);
}

int build_config(FILE *out) {
	char buf[4096], *p1, *p2;
	FILE *f;
	int i;

	/* hostname */
	gethostname(buf, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0'; /* paranoia :P */
	fprintf(out, "!\nhostname %s\n", buf);
	/* enable secrets */
	fputs("!\n", out);
	cfg_lock();
	for(i = 1; i < CLI_MAX_ENABLE; i++) {
		if(cfg->enable_secret[i][0] == '\0')
			continue;
		fprintf(out, "enable secret level %d 5 %s\n", i, cfg->enable_secret[i]);
	}
	if(cfg->enable_secret[CLI_MAX_ENABLE][0] != '\0') {
		fprintf(out, "enable secret 5 %s\n", cfg->enable_secret[i]);
	}
	cfg_unlock();
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
	fclose(f);
	
	/* virtual interfaces */	
	build_list_ip_addr(out, NULL, FMT_CMD);
#ifdef USE_EXIT_IN_CONF
	fprintf(out, "!\nexit\n");
#endif

	/* static macs */
	dump_static_macs(out);

	/* line vty stuff */
	fprintf(out, "!\nline vty 0 15\n");
	fprintf(out, " password %s\n", cfg->vty[0].passwd);
#ifdef USE_EXIT_IN_CONF
	fprintf(out, "!\nexit\n");
#endif

	return 0;
}

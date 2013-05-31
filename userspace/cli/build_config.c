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
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "climain.h"
#include "switch.h"
#include "ip.h"
#include "cdp_client.h"
#include "rstp_client.h"

/* extern functions from cdp.c */
extern int get_cdp_configuration(struct cdp_configuration *);
extern int cdp_if_is_enabled(char *);

void list_interface_status(FILE *out, char *dev) {
	int sockfd;
	struct ifreq ifr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return;
	}
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");
		close(sockfd);
		return;
	}
	if (!(ifr.ifr_flags & IFF_UP)) 
		fprintf(out, " shutdown\n");
	close(sockfd);
}

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

int build_int_eth_config(FILE *out, char *if_name, int put_if_cmd) {
	struct swcfgreq ioctl_arg;
	unsigned char bmp[SW_VLAN_BMP_NO];
	char desc[SW_MAX_PORT_DESC];
	int need_trunk_vlans = 1;

	ioctl_arg.cmd = SWCFG_GETIFCFG;
	ioctl_arg.ifindex = if_name;
	ioctl_arg.ext.cfg.forbidden_vlans = bmp;
	ioctl_arg.ext.cfg.description = desc;
	if(ioctl(sock_fd, SIOCSWCFG, &ioctl_arg) == -1)
		return errno;

	if (put_if_cmd)
		fprintf(out, "!\ninterface %s\n", if_name);
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
	if(ioctl_arg.ext.cfg.flags & IF_MODE_ACCESS)
		fprintf(out, " switchport mode access\n");
	if(ioctl_arg.ext.cfg.flags & IF_MODE_TRUNK)
		fprintf(out, " switchport mode trunk\n");
	/* speed */
	if(ioctl_arg.ext.cfg.speed != SW_SPEED_AUTO) {
		const char *speed = NULL;
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
		const char *duplex = NULL;
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
	/* cdp status (enabled/disabled). by default cdp is enabled */
	if (!cdp_if_is_enabled(if_name))
		fprintf(out, " no cdp enable\n");

	return 0;
}

#define INITIAL_BUF_SIZE 4096
void dump_static_macs(FILE *out) {
	char *buf, *ptr;
	int status, size;
	struct swcfgreq ioctl_arg;

	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	assert(buf);
	ioctl_arg.ifindex = NULL;
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

void dump_cdp_global_settings(FILE *out) {
	struct cdp_configuration conf;

	if (!cdp_s.enabled) {
		fprintf(out, "no cdp run\n");
		return;
	}
	if (get_cdp_configuration(&conf))
		return;
	if (conf.version != CFG_DFL_VERSION)
		fprintf(out, "no cdp advertise-v2\n");
	if (conf.timer != CFG_DFL_TIMER)
		fprintf(out, "cdp timer %d\n", conf.timer);
	if (conf.holdtime != CFG_DFL_HOLDTIME)
		fprintf(out, "cdp holdtime %d\n", conf.holdtime);
}

int pretty_print(FILE *out, struct list_head *ipl) {
	int count = 0;
	struct ip_addr_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, ipl, lh) {
		fprintf(out, " ip address %s %s", entry->inet, entry->mask);
		count++;
		if (count > 1)
			fprintf(out, " secondary");
		fprintf(out, "\n");
		list_del(&entry->lh);
		free(entry);
	}

	return count;
}

void build_list_ip_addr(FILE *out, char *dev) {
	FILE *fh;
	char buf[128];
	struct list_head *ipl;
	int vlan;
	
	/* No interface specified, we print out all ip 
	 information about lms virtual interfaces */
	fh = fopen(PROCNETSWITCH_PATH, "r");
	if (!fh) {
		perror("fopen");
		return;
	}
	while (fgets(buf, sizeof(buf), fh)) {
		buf[strlen(buf)-1] = '\0';
		if (dev && strcmp(dev, buf))
			continue;
		sscanf(buf, "vlan%d", &vlan);
		fprintf(out, "interface vlan %d\n", vlan);		
		ipl = list_ip_addr(buf, 0);
		if (!ipl || list_empty(ipl))
			fprintf(out, " no ip address\n");
		else 
			pretty_print(out, ipl);
		if (ipl)
			free(ipl);
		list_interface_status(out, buf);
		fprintf(out, "!\n");
		if (dev)
			break;	
	}
	fclose(fh);
}

void dump_vlans(FILE *out) {
	char *buf;
	int status, size;
	struct swcfgreq ioctl_arg;

	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	assert(buf);
	ioctl_arg.ifindex = NULL;
	ioctl_arg.cmd = SWCFG_GETVDB;

	do {
		ioctl_arg.ext.varg.buf_size = size;
		ioctl_arg.ext.varg.buf = buf;
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
	
	{
		int i, cnt = 0;
		struct net_switch_usr_vdb *entry = (struct net_switch_usr_vdb *)buf;
		for(i = 0; i < ioctl_arg.ext.varg.vdb_entries; entry++, i++) {
			if(sw_is_default_vlan(entry->vlan))
				continue;
			fprintf(out, "!\nvlan %d\n", entry->vlan);
			if(strcmp(entry->name, default_vlan_name(entry->vlan))) {
				fprintf(out, " name %s\n", entry->name);
			}
			cnt++;
		}
#ifdef USE_EXIT_IN_CONF
		if(cnt)
			fprintf(out, "exit\n");
#endif
	}
}

void dump_mac_aging_time(FILE *out) {
	long age;

	age = get_mac_age();
	if (age >= 0 && age != SW_DEFAULT_AGE_TIME)
		fprintf(out, "mac-address-table aging-time %li\n!\n", age);
}

int build_config(FILE *out, int include_tagged_if) {
	FILE *f;
	char buf[4096];
	int i;

	/* hostname */
	gethostname(buf, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0'; /* paranoia :P */
	fprintf(out, "!\nhostname %s\n", buf);

	/* enable secrets */
	fputs("!\n", out);
	cfg_lock();
	for(i = 1; i < CLI_MAX_ENABLE; i++) {
		if(CFG->enable_secret[i][0] == '\0')
			continue;
		fprintf(out, "enable secret level %d 5 %s\n", i, CFG->enable_secret[i]);
	}
	if(CFG->enable_secret[CLI_MAX_ENABLE][0] != '\0') {
		fprintf(out, "enable secret 5 %s\n", CFG->enable_secret[i]);
	}
	cfg_unlock();

	/* vlans (aka replacement for vlan database) */
	dump_vlans(out);
	
	/* physical interfaces */
	f = fopen("/proc/net/dev", "r");
	/* FIXME (blade) get interface list directly from switch via ioctl() */
	assert(f != NULL);
	while(fgets(buf, sizeof(buf), f) != NULL) {
		char *p;
		char tag[CLI_MAX_TAG + 1];
		int status = 0;

		/* parse interface name from /proc/net/dev line */
		if ((p = strchr(buf, ':')) == NULL)
			continue;
		*p = '\0';
		for(p = buf; *p == ' '; p++);

		do {
			char path[PATH_MAX];
			FILE *tag_file;

			if (cfg_get_if_tag(p, tag))
				break;

			printf("if_tag: if='%s' tag='%s'\n", p, tag);

			if (include_tagged_if) {
				fprintf(out, "!\n!interface %s is tagged as \"%s\"\n", p, tag);
				break;
			}

			if (snprintf(path, sizeof(path), "%s/%s", config_tags_path, tag) > sizeof(path)) {
				status = 1;
				break;
			}

			if (NULL == (tag_file = fopen(path, "w+"))) {
				status = 1;
				break;
			}

			build_int_eth_config(tag_file, p, 0);
			fclose(tag_file);
			status = 2;
		} while (0);

		switch (status) {
		case 1:
			fprintf(out, "!!cannot write separate config file for %s\n", p);
		case 2:
			continue;
		}

		build_int_eth_config(out, p, 1);
		fprintf(out, "!\n");
	}
	fclose(f);
	
	/* virtual interfaces */	
	build_list_ip_addr(out, NULL);
#ifdef USE_EXIT_IN_CONF
	fprintf(out, "exit\n");
#endif

	/* static macs */
	dump_static_macs(out);

	/* fdb mac aging time */
	dump_mac_aging_time(out);

	/* cdp global settings */
	dump_cdp_global_settings(out);

	/* line vty stuff */
	fprintf(out, "!\nline vty 0 15\n");
	if (strlen(CFG->vty[0].passwd))
		fprintf(out, " password %s\n", CFG->vty[0].passwd);
#ifdef USE_EXIT_IN_CONF
	fprintf(out, "exit\n");
#endif

	return 0;
}

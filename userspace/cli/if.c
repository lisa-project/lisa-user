#include "if.h"

LIST_HEAD(interfaces);

#define IFNUM_BASE 32

const struct {
    char	*name;
    unsigned short	value;
} media[] = {
    /* The order through 100baseT4 matches bits in the BMSR */
    { "10baseT-HD",	MII_AN_10BASET_HD },
    { "10baseT-FD",	MII_AN_10BASET_FD },
    { "100baseTx-HD",	MII_AN_100BASETX_HD },
    { "100baseTx-FD",	MII_AN_100BASETX_FD },
    { "100baseT4",	MII_AN_100BASET4 },
    { "100baseTx",	MII_AN_100BASETX_FD | MII_AN_100BASETX_HD },
    { "10baseT",	MII_AN_10BASET_FD | MII_AN_10BASET_HD },
};
#define NMEDIA (sizeof(media)/sizeof(media[0]))

/* Table of known MII's */
static struct {
    unsigned short id1, id2;
    char	*name;
} mii_id[] = {
    { 0x0022, 0x5610, "AdHoc AH101LF" },
    { 0x0022, 0x5520, "Altimata AC101LF" },
    { 0x0000, 0x6b90, "AMD 79C901A HomePNA" },
    { 0x0000, 0x6b70, "AMD 79C901A 10baseT" },
    { 0x0181, 0xb800, "Davicom DM9101" },
    { 0x0043, 0x7411, "Enable EL40-331" },
    { 0x0015, 0xf410, "ICS 1889" },
    { 0x0015, 0xf420, "ICS 1890" },
    { 0x0015, 0xf430, "ICS 1892" },
    { 0x02a8, 0x0150, "Intel 82555" },
    { 0x7810, 0x0000, "Level One LXT970/971" },
    { 0x2000, 0x5c00, "National DP83840A" },
    { 0x0181, 0x4410, "Quality QS6612" },
    { 0x0282, 0x1c50, "SMSC 83C180" },
    { 0x0300, 0xe540, "TDK 78Q2120" },
};
#define NMII (sizeof(mii_id)/sizeof(mii_id[0]))

int sockfd;

static int fetch_interface(struct user_net_device *);

static struct user_net_device *add_interface(char *ifname) {
	struct user_net_device *netdev;
	struct user_net_device *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &interfaces, lh) {
		if (!strncmp(entry->name, ifname, IFNAMSIZ)) {
			fetch_interface(entry);
			return entry;
		}	
	}	


	netdev = (struct user_net_device *)malloc(sizeof(struct user_net_device));
	if (!netdev) {
		printf("Out of memory!!!\n");
		return NULL;
	}		
	strncpy(netdev->name, ifname, IFNAMSIZ);
	fetch_interface(netdev);
	if (netdev->flags & IFF_LOOPBACK) {
		free(netdev);
		return NULL;
	}
	/* FIXME: sortare prin insertie ?? */
	list_add_tail(&netdev->lh, &interfaces);
	return netdev;
}

static void get_kern_comm() {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}
}

static void end_kern_comm() {
	close(sockfd);
}

static int get_interfaces_ifconf(void) {
	int len, ifnum, ret, i;
	struct ifreq *ifreq;
	struct ifconf ifconf;


	ifnum = IFNUM_BASE;
	ifconf.ifc_buf = NULL;
	len = 0;
	
	/* Loop until SIOCGIFCONF sucess */
	for (;;) {
		ifconf.ifc_len = sizeof(struct ifreq *) * ifnum;
		ifconf.ifc_buf = realloc(ifconf.ifc_buf, ifconf.ifc_len);

		if (!ifconf.ifc_buf) {
			perror("realloc");
			close(sockfd);
			return -1;
		}
			
		ret = ioctl(sockfd, SIOCGIFCONF, &ifconf);

		if (ret < 0) {
			perror("SIOCGIFCONF");
			goto out;
		}

		/* Repeatedly get info until buffer fails to grow */
		if (ifconf.ifc_len > len) {
			len = ifconf.ifc_len;
			ifnum += 10;
			continue;
		}

		/* Success */
		break;
	}

	/* Allocate interface */
	ifreq = ifconf.ifc_req;

	for (i = 0; i < ifconf.ifc_len; i+= sizeof(struct ifreq)) {
		/* FIXME: verificare sa fie virtuala / in switch */
		add_interface(ifreq->ifr_name);
		ifreq++;
	}

out:
	close(sockfd);
	free(ifconf.ifc_buf);

	return ret;
}

static char *get_dev_name(char *name, char *buf) {
	while (isspace(*buf))
		buf++;
	while (*buf) {
		if (isspace(*buf)) 
			break;
		if (*buf == ':') {
			buf++;
			break;	
		}	
		*name++ = *buf++;
	}
	*name++ = '\0';
	return buf;
}

static void get_dev_xstats(char *pos, struct user_net_device *iface) {
	sscanf(pos,
	"%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
	       &iface->xstats.rx_bytes,
	       &iface->xstats.rx_packets,
	       &iface->xstats.rx_errors,
	       &iface->xstats.rx_dropped,
	       &iface->xstats.rx_fifo_errors,
	       &iface->xstats.rx_frame_errors,
	       &iface->xstats.rx_compressed,
	       &iface->xstats.multicast,

	       &iface->xstats.tx_bytes,
	       &iface->xstats.tx_packets,
	       &iface->xstats.tx_errors,
	       &iface->xstats.tx_dropped,
	       &iface->xstats.tx_fifo_errors,
	       &iface->xstats.collisions,
	       &iface->xstats.tx_carrier_errors,
	       &iface->xstats.tx_compressed);
}

static int get_virtual_interfaces() {
	FILE *fh;
	char buf[512];

	fh = fopen(PROCNETSWITCH_PATH, "r");
	if (!fh) {
		perror("fopen");
		exit(-1);
	}

	while (fgets(buf, sizeof(buf), fh)) {
		buf[strlen(buf)-1] = '\0';
		add_interface(buf);
	}

	return 0;
}

static int get_interfaces_proc(void) {
	FILE *fh;
	int ret;
	char buf[512];
	char name[IFNAMSIZ], *s;
	char desc[SW_MAX_PORT_DESC];
	struct user_net_device *iface;
	struct net_switch_ioctl_arg ioctl_arg;

	fh = fopen(PROCNETDEV_PATH, "r");
	if (!fh) {
		printf("Cannot open /proc/net/dev !\n");
		return get_interfaces_ifconf();
	}
	/* read 2 header lines */
	fgets(buf, sizeof(buf), fh); 
	fgets(buf, sizeof(buf), fh);
	while (fgets(buf, sizeof(buf), fh)) {
		s = get_dev_name(name, buf);
		ioctl_arg.cmd = SWCFG_GETIFCFG;
		ioctl_arg.if_name = name;
		ioctl_arg.ext.cfg.forbidden_vlans = NULL;
		ioctl_arg.ext.cfg.description = desc;
		iface = NULL;
		ret = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
		/* Daca e interfata virtuala sau e in switch */
		if (!ret || !strncmp(name, LMS_VIRT_PREFIX, strlen(LMS_VIRT_PREFIX)))
			iface = add_interface(name);
		if (!iface)
			continue;
		memset(iface->desc, 0, SW_MAX_PORT_DESC);
		if (!ret) {
			if (desc && strlen(desc))
				strncpy(iface->desc, desc, SW_MAX_PORT_DESC);
		}	
		get_dev_xstats(s, iface);
	}
	fclose(fh);
	return 0;
}

static int mdio_read(struct ifreq *ifr, int location) {
    struct mii_data *mii = (struct mii_data *)&ifr->ifr_data;

    mii->reg_num = location;
	if (ioctl(sockfd, SIOCGMIIREG, ifr) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed\n", ifr->ifr_name);
		perror("ioctl");
		return -1;
	}
    return mii->val_out;
}

static int get_basic_mii(struct ifreq *ifr, struct user_net_device *iface) {
	int i;

//	printf("PHY_ID: %d\n", iface->phy_id);

	mdio_read(ifr, MII_BMSR);

	for (i=0; i<MII_NUM_REGS; i++) {
		iface->mii_regs[i] = mdio_read(ifr, i);
//		printf("mii_regs[%d]: 0x%x\n", i, iface->mii_regs[i]);
	}	

	if (iface->mii_regs[MII_BMCR] == 0xffff) {
		iface->mii_cap = 0;	
		return MII_NO_TRANSCEIVER;
	}

	return 0;
}

static void get_mii_props(struct user_net_device *iface) {
	struct ifreq ifr;
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;

	strncpy(ifr.ifr_name, iface->name, IFNAMSIZ);
	if ((ioctl(sockfd, SIOCGMIIPHY, &ifr) < 0))
		iface->mii_cap = 0;
	else {
		iface->mii_cap = 1;
		iface->phy_id = mii->phy_id;
		get_basic_mii(&ifr, iface);
	}
}

static int fetch_interface(struct user_net_device *iface) {
	struct ifreq ifr;

	strcpy(ifr.ifr_name, iface->name);
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
		return -1;
	iface->flags = ifr.ifr_flags;

	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
		memset(iface->mac, 0, sizeof(iface->mac));
	else 
		memcpy(iface->mac, ifr.ifr_hwaddr.sa_data, sizeof(iface->mac));

	if (ioctl(sockfd, SIOCGIFMETRIC, &ifr) < 0)
		iface->metric = 0;
	else
		iface->metric = ifr.ifr_metric;
	
	if (ioctl(sockfd, SIOCGIFMTU, &ifr) < 0)
		iface->mtu = 0;
	else
		iface->mtu = ifr.ifr_mtu;

	if (ioctl(sockfd, SIOCGIFMAP, &ifr) < 0)
		memset(&iface->map, 0, sizeof(struct ifmap));
	else
		iface->map = ifr.ifr_map;

	if (ioctl(sockfd, SIOCGIFTXQLEN, &ifr) < 0)
		iface->tx_queue_len = -1;
	else 
		iface->tx_queue_len = ifr.ifr_qlen;
	
	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
		iface->has_ip = 1;
		iface->addr = ifr.ifr_addr;
		if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0)
			memset(&iface->broadaddr, 0, sizeof(struct sockaddr));
		else
			iface->broadaddr = ifr.ifr_broadaddr;
		if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0)
			memset(&iface->netmask, 0, sizeof(struct sockaddr));
		else
			iface->netmask = ifr.ifr_netmask;
	}
	else 
		memset(&iface->addr, 0, sizeof(struct sockaddr));


	get_mii_props(iface);

	/* FIXME: hwslip? outfill? keepalive? (SIOCGOUTFILL & SIOCGKEEPALIVE) */
	/* FIXME: AppleTalk, IPX, EcoNet? */

	return 0;
}

static char *media_list(int mask, int best) {
    static char buf[100];
    int i;
    *buf = '\0';
    mask >>= 5;
    for (i = 4; i >= 0; i--) {
	if (mask & (1<<i)) {
	    strcat(buf, " ");
	    strcat(buf, media[i].name);
	    if (best) break;
	}
    }
    if (mask & (1<<5))
	strcat(buf, " flow-control");
    return buf;
}
void sh_iface_print(FILE *out, struct user_net_device *entry) {
	int bmcr, bmsr, advert, lkpar;
	int virtual, line_proto_down, i;

	virtual = 0;
	line_proto_down = 1;
	fprintf(out, "%s ", entry->name);
	fprintf(out, "is %s, ", (entry->flags & IFF_UP)?"up":"down");
	if (entry->mii_cap) {
		bmcr = entry->mii_regs[MII_BMCR];
		bmsr = entry->mii_regs[MII_BMSR];
		advert = entry->mii_regs[MII_ANAR];
		lkpar = entry->mii_regs[MII_ANLPAR];
	}
	fprintf(out, "line protocol is ");
	if (!strncmp(entry->name, LMS_VIRT_PREFIX, 
				strlen(LMS_VIRT_PREFIX))) {
		fprintf(out, "%s\n", (entry->flags & IFF_UP)?"up":"down");
		virtual = 1;
		line_proto_down = 0;
	}	
	else if (entry->mii_cap) {
		fprintf(out, "%s\n", (bmsr & MII_BMSR_LINK_VALID)?"up":"down");
		line_proto_down = !(bmsr & MII_BMSR_LINK_VALID);
	}	
	else { /* FIXME: are we optimistic or what? ;-) */
		fprintf(out, "down\n");
		line_proto_down = 1;
	}	

	fprintf(out, "  Hardware is %s, ", 
			(virtual)? "CPU Interface":"Fast Ethernet");
	fprintf(out,"address is %02hhx%02hhx.%02hhx%02hhx.%02hhx%02hhx "
			"(bia %02hhx%02hhx.%02hhx%02hhx.%02hhx%02hhx)\n" ,
			entry->mac[0], entry->mac[1], entry->mac[2],
			entry->mac[3], entry->mac[4], entry->mac[5],
			entry->mac[0], entry->mac[1], entry->mac[2],
			entry->mac[3], entry->mac[4], entry->mac[5]
		   );
	if (entry->desc && strlen(entry->desc))
		fprintf(out, "  Description: %s\n", entry->desc);
	fprintf(out, "  MTU %d bytes, ", entry->mtu);
	if (virtual)
		fprintf(out, "BW 10000 Kbit, DLY 1000 usec,\n");
	else if (entry->mii_cap) {
		/* FIXME: check if true (if autonegotiation enabled) */
		fprintf(out, "BW %s Kbit, DLY %s usec,\n",
				(bmcr & MII_BMCR_100MBIT)? "100000":"10000",
				(bmcr & MII_BMCR_100MBIT)? "100":"1000");
	} 
	else 
		fprintf(out, "BW unknown, DLY unknown,\n");
	/* Next 2 just for fun */
	fprintf(out,"  Encapsulation ARPA, loopback not set\n");
	if (!virtual) {
		fprintf(out, "  Keepalive not set\n");
		if (entry->mii_cap && !line_proto_down) {
			if (bmcr & MII_BMCR_AN_ENA) {
					if (bmsr & MII_BMSR_AN_COMPLETE) { 
						if (advert & lkpar) {
							if (lkpar & MII_AN_ACK)
								fprintf(out, "  Autonegotiation enabled, ");
							else
								fprintf(out, "  No autonegotiation, ");
							fprintf(out, "%s\n",	media_list(advert&lkpar,1));
						}
						else 
							fprintf(out, "  Autonegotiation failed\n");
					}		
					else if (bmcr & MII_BMCR_RESTART)
						fprintf(out, "  Autonegotiation restarted\n");
			}	
			else 
				fprintf(out, "  %s Duplex, Speed %s Mbit, %s\n",
						(bmcr & MII_BMCR_100MBIT)? "100":"10",
						(bmcr & MII_BMCR_DUPLEX)? "Full": "Half",
						(advert & lkpar)? media_list(advert&lkpar,1):
						"unknown");
			fprintf(out, "  Product info: ");
			for (i = 0; i < NMII; i++)
				if ((mii_id[i].id1 == entry->mii_regs[2]) &&
				(mii_id[i].id2 == (entry->mii_regs[3] & 0xfff0)))
				break;
			if (i < NMII)
				fprintf(out, "%s rev %d\n", mii_id[i].name, entry->mii_regs[3]&0x0f);
			else
				fprintf(out, "vendor %02x:%02x:%02x, model %d rev %d\n",
				   entry->mii_regs[2]>>10, (entry->mii_regs[2]>>2)&0xff,
				   ((entry->mii_regs[2]<<6)|(entry->mii_regs[3]>>10))&0xff,
				   (entry->mii_regs[3]>>4)&0x3f, entry->mii_regs[3]&0x0f);
			fprintf(out, "  basic mode:   ");
			if (bmcr & MII_BMCR_RESET)
				fprintf(out, "software reset, ");
			if (bmcr & MII_BMCR_LOOPBACK)
				fprintf(out, "loopback, ");
			if (bmcr & MII_BMCR_ISOLATE)
				fprintf(out, "isolate, ");
			if (bmcr & MII_BMCR_COLTEST)
				fprintf(out, "collision test, ");
			if (bmcr & MII_BMCR_AN_ENA) {
				fprintf(out, "autonegotiation enabled\n");
			} else {
				fprintf(out, "%s Mbit, %s duplex\n",
				   (bmcr & MII_BMCR_100MBIT) ? "100" : "10",
				   (bmcr & MII_BMCR_DUPLEX) ? "full" : "half");
			}
			fprintf(out, "  basic status: ");
			if (bmsr & MII_BMSR_AN_COMPLETE)
				fprintf(out, "autonegotiation complete, ");
			else if (bmcr & MII_BMCR_RESTART)
				fprintf(out, "autonegotiation restarted, ");
			if (bmsr & MII_BMSR_REMOTE_FAULT)
				fprintf(out, "remote fault, ");
			fprintf(out, (bmsr & MII_BMSR_LINK_VALID) ? "link ok" : "no link");
			fprintf(out, "\n  capabilities:%s", media_list(bmsr >> 6, 0));
			fprintf(out, "\n  advertising: %s", media_list(advert, 0));
			if (lkpar & MII_AN_ABILITY_MASK)
				fprintf(out, "\n  link partner:%s", media_list(lkpar, 0));
			fprintf(out, "\n");
		}	
	}	
	fprintf(out,"  ARP type: ARPA, ARP Timeout 04:00:00\n");
	if (entry->has_ip) {
		print_sockaddr_ip(out, "  inet addr", entry->addr);
		print_sockaddr_ip(out, "bcast", entry->broadaddr);
		print_sockaddr_ip(out, "netmask", entry->netmask);
		fprintf(out, "\n");
	}
	fprintf(out, "  Metric %d, Tx queue len: %d\n",
			entry->metric, entry->tx_queue_len);
	if (entry->map.base_addr)
		fprintf(out, "  Base addr: 0x%hx IRQ: %d DMA: %d\n",
				entry->map.base_addr,
				entry->map.irq,
				entry->map.dma);
	fprintf(out, "\trx bytes: %lu rx packets: %lu tx bytes: %lu "
			"tx packets: %lu\n",
			entry->xstats.rx_bytes, entry->xstats.rx_packets,
			entry->xstats.tx_bytes, entry->xstats.tx_packets);
	fprintf(out, "\trx errors: %lu rx dropped: %lu tx errors: %lu "
			"tx dropped: %lu\n",
			entry->xstats.rx_errors, entry->xstats.rx_dropped,
			entry->xstats.tx_errors, entry->xstats.tx_dropped);
	fprintf(out, "\trx fifo errors: %lu rx frame errors: %lu "
			"rx compressed: %lu\n",
			entry->xstats.rx_fifo_errors, entry->xstats.rx_frame_errors,
			entry->xstats.rx_compressed);
	fprintf(out, "\ttx_fifo errors: %lu collisions: %lu "
			"tx carrier errors: %lu tx compressed: %lu\n",
			entry->xstats.tx_fifo_errors, entry->xstats.collisions,
			entry->xstats.tx_carrier_errors, entry->xstats.tx_compressed);
}

static void do_cleanup_ifaces() {
	struct user_net_device *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &interfaces, lh) {
		list_del(&entry->lh);
		free(entry);
	}
}

static void do_get_ifaces() {
	do_cleanup_ifaces();
	INIT_LIST_HEAD(&interfaces);
	get_kern_comm();
    get_virtual_interfaces();
	get_interfaces_proc();
	end_kern_comm();
}

void cmd_int_eth(FILE *out, char **argv) {
	char *arg = *argv;
	struct user_net_device *entry, *tmp;
	int eth_no = parse_eth(arg), n;
	char buf[IFNAMSIZ];

	n = sprintf(buf, "eth%d", eth_no);
	assert(n < IFNAMSIZ);
	do_get_ifaces();

	list_for_each_entry_safe(entry, tmp, &interfaces, lh) {
		if (!strcmp(entry->name, buf)) {
			sh_iface_print(out, entry);
			break;
		}
	}
}

void cmd_int_vlan(FILE *out, char **argv) {
	char *arg = *argv;
	struct user_net_device *entry, *tmp;
	int vlan_no = parse_vlan(arg), n;
	char buf[IFNAMSIZ];

	n = sprintf(buf, "vlan%d", vlan_no);
	assert(n < IFNAMSIZ);
	do_get_ifaces();

	list_for_each_entry_safe(entry, tmp, &interfaces, lh) {
		if (!strcmp(entry->name, buf)) {
			sh_iface_print(out, entry);
			break;
		}
	}
}

void cmd_sh_int(FILE *out, char **argv) {
	struct user_net_device *entry, *tmp;

	do_get_ifaces();
	list_for_each_entry_safe(entry, tmp, &interfaces, lh) {
		sh_iface_print(out, entry);
		fprintf(out, "\n");
	}
}
/*
int main(int argc, char *argv[]) {
	get_kern_comm();
	get_interfaces_proc();
	end_kern_comm();
	cmd_sh_int(stdout, NULL);
	return 0;
} */

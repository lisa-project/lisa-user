/*
 *    This file is part of LiSA Switch
 *
 *    LiSA Switch is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2 of the
 *    License, or (at your option) any later version.
 *
 *    LiSA Switch is distributed in the hope that it will be
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Switch; if not, write to the Free
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 */
#include "socket_api.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

int register_filter(pcap_t **pcap, const char* filter)
{
	int ret;
	char *dev;
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 netp;
	bpf_u_int32 maskp;
	struct bpf_program fp;

	/* grab the device*/
	dev = pcap_lookupdev(errbuf);
	if (NULL == dev) {
		printf("Error pcap_lookupdev(): %s\n", errbuf);
		return -1;
	}

	/* get network address and mask of the device */
	ret = pcap_lookupnet(dev, &netp, &maskp, errbuf);
	if (ret < 0) {
		printf("Error pcap_lookupnet(): %s\n", errbuf);
		return ret;
	}

	/* open device for monitoring */
	*pcap = pcap_open_live(dev, PACKET_SIZE, 1, -1, errbuf);
	if (NULL == *pcap) {
		printf("Error pcap_open_live(): %s\n", errbuf);
		return -1;
	}

	/* compile the program */
/*	ret = pcap_compile(*pcap, &fp, filter, 0, netp);
	if (ret < 0) {
		printf("Error pcap_compile()");
		return ret;
	}*/

	/* set compiled program as filter */
/*	ret = pcap_setfilter(*pcap, &fp);
	if (ret < 0) {
		printf("Error pcap_setfilter()");
		return -1;
	}*/

	return 0;
}

u_char* recv_packet(pcap_t *pcap)
{
	struct pcap_pkthdr header;
	return (u_char*)pcap_next(pcap, &header);
}

int send_packet(int sock_fd, unsigned char *packet, int size)
{
	return send(sock_fd, packet, size, 0);
}

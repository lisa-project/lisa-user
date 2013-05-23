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
#ifndef _SOCKET_API_H
#define _SOCKET_API_H

#define PACKET_SIZE	65536
#define CDP_FILTER	"ether multicast and ether[20:2] = 0x2000"

#include <pcap.h>

int register_filter(pcap_t **pcap, const char* filter);

int send_packet(int sock_fd,  unsigned char *packet, int size);

u_char* recv_packet(pcap_t *pcap);

#endif

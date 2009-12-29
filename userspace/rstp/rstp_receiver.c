#include "rstp.h"
#include "shared.h"

extern struct list_head registered_interfaces;

void*
rstp_recv_loop(void* params)
{
	struct rstp_interface *entry, *tmp;
	int fd, maxfd, status, len;
	fd_set rdfs;

	for (;;) {
		/* rebuild descriptor set */
		FD_ZERO(&rdfs);
		maxfd = -1;
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			fd = entry->sw_sock_fd;
			FD_SET(fd, &rdfs);
			if (fd > maxfd)
				maxfd = fd;
		}

		/* no descriptors means no interfaces have RSTP enabled */
		if (maxfd < 0) {
			continue;
		}

		/* wait for activity on any of the monitored interfaces */
		status = select(maxfd + 1, &rdfs, 0, 0, 0);
		if (status < 0)
			continue;

		/* iterate through all interfaces and get any available data */
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			if (!FD_ISSET(entry->sw_sock_fd, &rdfs))
				continue;
			//TODO: malformed bpdu check + size
			/* if the interface has not finished processing its
			 * BPDU then do not get the received BPDU (yet) */
			if (sem_trywait(&entry->processedBPDU))
				continue;

			if ((len = recv(entry->sw_sock_fd, entry->buf, 60, 0)) < 0) {
				sys_dbg("[rstp receiver]: error rcv BPDU");
				continue;
			}

			sys_dbg("[rstp receiver]: data received on interface %d, size %d\n", entry->if_index, len);
			//printf("%02x%02x%02x%02x\n", entry->buf[56], entry->buf[57], entry->buf[58], entry->buf[59]);

			/* signal the Port Receive State Machine that a new
			 * BPDU has arrived */
			sem_post(&entry->rcvdBPDU);
		}
	}

	return NULL;
}


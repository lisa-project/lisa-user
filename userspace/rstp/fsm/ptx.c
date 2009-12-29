/* PORT TRANSMIT MACHINE */

#include "rstp.h"

#define PTX_TRANSMIT_INIT	0
#define PTX_IDLE		1
#define PTX_TRANSMIT_PERIODIC	2
#define PTX_TRANSMIT_CONFIG	3
#define PTX_TRANSMIT_TCN	4
#define PTX_TRANSMIT_RSTP	5

unsigned char rstp_multicast[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};

void
fill_bpdu_mac(struct bpdu_t *bpdu, struct rstp_interface *port)
{
	memcpy(bpdu->mac.dst_mac, rstp_multicast, 6);
	memcpy(bpdu->mac.src_mac, port->mac_addr, 6);
}

void
fill_bpdu_eth(struct bpdu_t *bpdu)
{
	bpdu->eth.dsap = 0x42;
	bpdu->eth.ssap = 0x42;
	bpdu->eth.llc = 0x03;
	bpdu->eth.len[0] = 0; bpdu->eth.len[1] = 43;
}

void txConfig(struct rstp_interface *port)
{
	unsigned char flags = 0;
	struct bpdu_t bpdu;

	fill_bpdu_mac(&bpdu, port);
	fill_bpdu_eth(&bpdu);

	*(unsigned short*)bpdu.hdr.protocol = 0;
	bpdu.hdr.version = 0;
	bpdu.hdr.bpdu_type = BPDU_CONFIG;

	memcpy(bpdu.body.root_id, &port->designatedPriority, sizeof(struct priority_vector4));

	flags |= (port->tcWhile != 0) ? TOPOLOGY_CHANGE_BIT : 0;
	flags |= (port->tcAck) ? TOPOLOGY_CHANGE_ACK_BIT : 0;
	bpdu.body.flags = flags;

	memcpy(bpdu.body.message_age, &port->designatedTimes, sizeof(struct rstp_times));

	if (send(port->sw_sock_fd, &bpdu, 35 + 17, 0) < 0) {
		Dprintf("Could not send\n");
	}
	//Dprintf("sent config\n");
}

void txTcn(struct rstp_interface *port)
{
	struct bpdu_t bpdu;

	fill_bpdu_mac(&bpdu, port);
	fill_bpdu_eth(&bpdu);

	*(unsigned short*)bpdu.hdr.protocol = 0;
	bpdu.hdr.version = 0;
	bpdu.hdr.bpdu_type = BPDU_TCN;

	if (send(port->sw_sock_fd, &bpdu, 4 + 17, 0) < 0) {
		Dprintf("Could not send\n");
	}

	//Dprintf("sent tcn\n");
}

void txRstp(struct rstp_interface *port)
{
	unsigned char flags = 0;
	struct bpdu_t bpdu;

	fill_bpdu_mac(&bpdu, port);
	fill_bpdu_eth(&bpdu);

	*(unsigned short*)bpdu.hdr.protocol = 0;
	bpdu.hdr.version = 2;
	bpdu.hdr.bpdu_type = BPDU_RSTP;

	memcpy(bpdu.body.root_id, &port->designatedPriority, sizeof(struct priority_vector4));

	if (port->role == ROLE_BACKUP || port->role == ROLE_ALTERNATE)
		flags |= ROLE_ALTERNATE_BACKUP;
	else
		flags |= port->role;

	flags |= port->agree ? AGREEMENT_BIT : 0;
	flags |= port->proposing ? PROPOSAL_BIT : 0;
	flags |= (port->tcWhile != 0) ? TOPOLOGY_CHANGE_BIT : 0;
	flags |= port->learning ? LEARN_BIT : 0;
	flags |= port->forwarding ? FORWARD_BIT : 0;
	bpdu.body.flags = flags;

	memcpy(bpdu.body.message_age, &port->designatedTimes, sizeof(struct rstp_times));

	bpdu.ver_1_len = 0;

	if (send(port->sw_sock_fd, &bpdu, 36 + 17, 0) < 0) {
		Dprintf("Could not send\n");
	}

	//Dprintf("sent rstp\n");
}

void (*ptx_state_table[6])(struct rstp_interface *) = {ptx_transmit_init, ptx_idle, ptx_transmit_periodic,
	ptx_transmit_config, ptx_transmit_tcn, ptx_transmit_rstp};

void
ptx_transmit_init(struct rstp_interface *port)
{
	//Dprintf("PTX_INIT\n");
	port->newInfo = 1;
	port->txCount = 0;

	port->state[PTX][FUNC] = PTX_IDLE;
	port->state[PTX][EXEC] = NOT_EXECUTED;
}

void
ptx_idle(struct rstp_interface *port)
{
	int cond;
	//Dprintf("PTX_IDLE\n");

	if (!port->state[PTX][EXEC]) {
		port->helloWhen = 2;
	}

	cond = port->newInfo && (port->txCount < 6) && port->helloWhen && port->selected && !port->updtInfo;

	if (port->sendRSTP && cond) {
		port->state[PTX][FUNC] = PTX_TRANSMIT_RSTP;
		port->state[PTX][EXEC] = NOT_EXECUTED;
	} else if (!port->sendRSTP && (port->role == ROLE_ROOT) && cond) {
		port->state[PTX][FUNC] = PTX_TRANSMIT_TCN;
		port->state[PTX][EXEC] = NOT_EXECUTED;
	} else if (!port->sendRSTP && (port->role == ROLE_DESIGNATED) && cond) {
		port->state[PTX][FUNC] = PTX_TRANSMIT_CONFIG;
		port->state[PTX][EXEC] = NOT_EXECUTED;
	} else if (!port->helloWhen) {
		port->state[PTX][FUNC] = PTX_TRANSMIT_PERIODIC;
		port->state[PTX][EXEC] = NOT_EXECUTED;
	} else {
		port->state[PTX][EXEC] = EXECUTED;
	}
}

void
ptx_transmit_periodic(struct rstp_interface *port)
{
	//Dprintf("PTX_TRANSMIT_PERIODIC\n");
	port->newInfo = port->newInfo || ((port->role == ROLE_DESIGNATED) ||
		((port->role == ROLE_ROOT) && port->tcWhile));

	port->state[PTX][FUNC] = PTX_IDLE;
	port->state[PTX][EXEC] = NOT_EXECUTED;
}

void
ptx_transmit_config(struct rstp_interface *port)
{
	//Dprintf("PTX_TRANSMIT_CONFIG\n");
	port->newInfo = 0;
	txConfig(port);
	port->txCount += 1;
	port->tcAck = 0;

	port->state[PTX][FUNC] = PTX_IDLE;
	port->state[PTX][EXEC] = NOT_EXECUTED;
}

void
ptx_transmit_tcn(struct rstp_interface *port)
{
	//Dprintf("PTX_TRANSMIT_TCN\n");
	port->newInfo = 0;
	txTcn(port);
	port->txCount += 1;

	port->state[PTX][FUNC] = PTX_IDLE;
	port->state[PTX][EXEC] = NOT_EXECUTED;
}

void
ptx_transmit_rstp(struct rstp_interface *port)
{
	//Dprintf("PTX_TRANSMIT_RSTP\n");
	port->newInfo = 0;
	txRstp(port);
	port->txCount += 1;
	port->tcAck = 0;

	port->state[PTX][FUNC] = PTX_IDLE;
	port->state[PTX][EXEC] = NOT_EXECUTED;
}

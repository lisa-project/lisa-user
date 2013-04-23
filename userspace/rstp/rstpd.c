#include "rstp.h"
#include "swsock.h"
#include "switch.h"
#include <sys/time.h>

pthread_t mgmt_thread;
pthread_t recv_thread;
pthread_t sig_thread;

LIST_HEAD(registered_interfaces);

pthread_mutex_t list_lock;

extern char rstp_queue_name[32];

extern unsigned int checkGlobalConditions(struct rstp_interface *);

struct rstp_bridge bridge;

// TODO: drop this in favor of LISA's shm mechanism
static void
init_bridge(void)
{
	unsigned char addr[6] = {0x00,0xE0,0x81,0xB1,0xC0,0x38};
	unsigned char defaultPriority[2] = {0x70,0x01};

	memcpy(&bridge.BridgeIdentifier.bridge_priority, &defaultPriority, 2);
	memcpy(&bridge.BridgeIdentifier.bridge_address, addr, 6);

	memset(&bridge.BridgePriority, 0, sizeof(struct priority_vector));
	memcpy(&bridge.BridgePriority.root_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id));
	memcpy(&bridge.BridgePriority.designated_bridge_id, &bridge.BridgeIdentifier, sizeof(struct bridge_id));

	bridge.BridgeTimes.MessageAge = 0;
	bridge.BridgeTimes.MaxAge = 20;
	bridge.BridgeTimes.ForwardDelay = 15;
	bridge.BridgeTimes.HelloTime = 2;
}

static int
setup_switch_socket(int fd, char *ifname)
{
	struct sockaddr_sw addr;

	memset(&addr, 0, sizeof(addr));
	addr.ssw_family = AF_SWITCH;
	strncpy(addr.ssw_if_name, ifname, sizeof(addr.ssw_if_name) - 1);
	addr.ssw_proto = ETH_P_RSTP;
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("bind");
		close(fd);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	return 0;
}

#define dec(x) if (x) x = x - 1
void
pti_loop(int signum)
{
	struct rstp_interface *entry, *tmp;

	// Decrement all timers
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
		dec(entry->helloWhen);
		dec(entry->tcWhile);
		dec(entry->fdWhile);
		dec(entry->rcvdInfoWhile);
		dec(entry->rrWhile);
		dec(entry->rbWhile);
		dec(entry->mdelayWhile);
		dec(entry->edgeDelayWhile);
		dec(entry->txCount);
	}
}

void
rstpd_register_interface(int if_index)
{
	int i, sock, status;
	struct ifreq ifr;
	struct swcfgreq swcfgr;
	struct rstp_interface *entry, *tmp;
	struct rstp_configuration rstp;

	sys_dbg("Enabling RSTP on interface %d\n", if_index);

	/* Open a socket to request interface status information (up/down) */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock>=0);

	/* get interface name */
	if_get_name(if_index, sock, ifr.ifr_name);

	status = ioctl(sock, SIOCGIFFLAGS, &ifr);
	assert(status>=0);

	/* interface is down */
	if (!(ifr.ifr_flags & IFF_UP)) {
		sys_dbg("interface %s is down.\n", ifr.ifr_name);
		close(sock);
		return;
	}
	close(sock);

	/* Open a switch socket to check if the interface is in the switch */
	sock = socket(PF_SWITCH, SOCK_RAW, 0);
	assert(sock>=0);

	/* Check if interface is in the switch */
	swcfgr.cmd = SWCFG_GETIFTYPE;
	swcfgr.ifindex = if_index;
	if (ioctl(sock, SIOCSWCFG, &swcfgr)) {
		sys_dbg("interface %s is not in the switch.\n", ifr.ifr_name);
		perror("ioctl");
		close(sock);
		return;
	}

	if (swcfgr.ext.switchport != SW_IF_SWITCHED) {
		sys_dbg("interface %s is not known by the switch.\n", ifr.ifr_name);
		close(sock);
		return;
	}

	/* Check if RSTP is already enabled on that interface */
	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (if_index == entry->if_index) {
			sys_dbg("RSTP already enabled on interface %s.\n", ifr.ifr_name);
			close(sock);
			return;
		}

	if (setup_switch_socket(sock, ifr.ifr_name))
		return;

	// Allocate space for RSTP per-port information
	entry = (struct rstp_interface *) malloc(sizeof(struct rstp_interface));
	assert(entry);
	entry->if_index = if_index;
	entry->sw_sock_fd = sock;
	entry->operEdge = 0;

	entry->PortPathCost = 200000;

	entry->portId[0] = 0x80 | (if_index >> 8);
	entry->portId[1] = if_index & 0xff;

	status = ioctl(entry->sw_sock_fd, SIOCGIFHWADDR, &ifr);
	memcpy(entry->mac_addr, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

	switch_get_rstp(&rstp);

	memset(&entry->portPriority, 0, sizeof(struct priority_vector4));
	memcpy(&entry->portPriority.root_bridge_id, &bridge.BridgePriority.root_bridge_id, sizeof(struct bridge_id));
	memcpy(&entry->portPriority.designated_bridge_id, &bridge.BridgePriority.designated_bridge_id, sizeof(struct bridge_id));
	memcpy(&entry->portPriority.designated_port_id, &entry->portId, 2);

	memset(&entry->designatedPriority, 0, sizeof(struct priority_vector4));
	memcpy(&entry->designatedPriority.root_bridge_id, &bridge.BridgePriority.root_bridge_id, sizeof(struct bridge_id));
	memcpy(&entry->designatedPriority.designated_bridge_id, &bridge.BridgePriority.designated_bridge_id, sizeof(struct bridge_id));
	memcpy(&entry->designatedPriority.designated_port_id, &entry->portId, 2);

	// Initialize semaphores
	if (sem_init(&entry->rcvdBPDU, 0, 0) < 0) {
		close(sock);
		return;
	}

	if (sem_init(&entry->processedBPDU, 0, 1) < 0) {
		close(sock);
		return;
	}

	// Clear all FSM states
	for (i = 0; i < 9; i++) {
		entry->state[i][FUNC] = 0;
		entry->state[i][EXEC] = NOT_EXECUTED;
	}

	pthread_mutex_lock(&list_lock);
	list_add_tail(&entry->lh, &registered_interfaces);
	pthread_mutex_unlock(&list_lock);

	sys_dbg("RSTP enabled on interface %d\n", if_index);
}

void
rstpd_unregister_interface(int if_index) {
	struct rstp_interface *entry, *tmp;

	sys_dbg("Disabling RSTP on interface %d\n", if_index);

	list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh)
		if (if_index == entry->if_index) {
			close(entry->sw_sock_fd);
			list_del(&entry->lh);
		}
}

void *
signal_handler(void *ptr)
{
	sigset_t signal_set;
	int sig;

	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
	sigaddset(&signal_set, SIGTERM);
	sigaddset(&signal_set, SIGSEGV);

	sys_dbg("[signal handler]: Waiting for SIGINT|SIGTERM|SIGSEGV ...\n");
	sigwait(&signal_set, &sig);

	sys_dbg("[signal handler]: Caught signal ... \n");
	sys_dbg("[signal handler]: Removing the IPC queue, '%s' ... \n",
		rstp_queue_name);
	if (mq_unlink(rstp_queue_name) < 0)
		sys_dbg("[signal handler]: Could not remove IPC queue ...\n");

	sys_dbg("[signal handler]: Exiting \n");
	closelog();
	exit(EXIT_FAILURE);
}

void
fsm_run(void)
{
	struct rstp_interface *entry, *tmp;

	// Iterate through all FSMs and execute their current state
	for (;;) {
		pthread_mutex_lock(&list_lock);
		list_for_each_entry_safe(entry, tmp, &registered_interfaces, lh) {
			prx_state_table[ entry->state[PRX][FUNC] ](entry);
			ppm_state_table[ entry->state[PPM][FUNC] ](entry);
			//bdm_state_table[ entry->state[BDM][FUNC] ](entry);
			ptx_state_table[ entry->state[PTX][FUNC] ](entry);
			pim_state_table[ entry->state[PIM][FUNC] ](entry);
			checkGlobalConditions(entry);
			prt_state_table[ entry->state[PRT][FUNC] ](entry);
			pst_state_table[ entry->state[PST][FUNC] ](entry);
			tcm_state_table[ entry->state[TCM][FUNC] ](entry);
		}

		prs_state_table[ prs_func ]();
		pthread_mutex_unlock(&list_lock);
	}
}

int main(int argc, char *argv[])
{
	sigset_t signal_set;
	sigset_t alrm_mask;
	struct sigaction alrm_action;
	struct itimerval timer;

	/* All output shall be sent to rsyslogd */
	openlog("rstpd", LOG_PID, LOG_DAEMON);
	daemonize();

	/* Mask all signals */
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

	//TODO:fix
	init_bridge();

	/* Initialize shared memory area */
	switch_init();

	/* Set one second timer */
	sigemptyset(&alrm_mask);
	sigaddset(&alrm_mask, SIGALRM);
	alrm_action.sa_flags = 0;
	alrm_action.sa_mask = alrm_mask;
	alrm_action.sa_handler = pti_loop;
	sigaction(SIGALRM, &alrm_action, NULL);

	memset(&timer, 0, sizeof(timer));
	timer.it_value.tv_sec = 1;
	timer.it_interval.tv_sec = 1;

        setitimer(ITIMER_REAL, &timer, NULL);

	pthread_mutex_init(&list_lock, NULL);

	/* Start frame receiving thread */
	if (pthread_create(&recv_thread, NULL, rstp_recv_loop, NULL) != 0) {
		sys_dbg("Could not create receiving thread");
		exit(EXIT_FAILURE);
	}

	/* Start management thread */
	if (pthread_create(&mgmt_thread, NULL, rstp_ipc_listen, NULL) != 0) {
		sys_dbg("Could not create management thread");
		exit(EXIT_FAILURE);
	}

	/* Start signal handler thread */
	if (pthread_create(&sig_thread, NULL, signal_handler, NULL) != 0) {
		perror("Could not create signal handler thread");
		exit(EXIT_FAILURE);
	}

	sys_dbg("Daemon started");

	/* Run finite state machines */
	fsm_run();

	/* Normally, execution never reaches this point */

	return EXIT_SUCCESS;
}

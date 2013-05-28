TOP_DIR = ..

CGLAGS	= -g
include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include \

SRCS	= vlan_rename.c pktgen.c pktrecv.c test_sock.c mcast_recv.c mcast_send.c test_sock_api.c
SIMPLE_TARGETS = pktgen pktrecv test_sock mcast_recv mcast_send
TARGETS	= $(SIMPLE_TARGETS) vlan_rename test_sock_api

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

vlan_rename: vlan_rename.o $(TOP_DIR)/switch/libswitch.so
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) -o $@ $^ ../lib/if_generic.o \
		../lib/netlink.o ../lib/util.o ../lib/mm.o -lrt $(LDFLAGS)

test_sock_api: %: %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ ../lib/socket_api.o -lpcap


$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

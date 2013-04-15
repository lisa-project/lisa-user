TOP_DIR = ..

CGLAGS	= -g
include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include

SRCS	= vlan_rename.c pktgen.c pktrecv.c test_sock.c mcast_recv.c mcast_send.c
SIMPLE_TARGETS = pktgen pktrecv test_sock mcast_recv mcast_send
TARGETS	= $(SIMPLE_TARGETS) vlan_rename

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

vlan_rename: vlan_rename.o $(TOP_DIR)/switch/libswitch.so
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/mm.o -lrt


$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

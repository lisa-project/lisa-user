TOP_DIR = ..

CGLAGS	= -g
include $(TOP_DIR)/scripts/Makefile.includes
INC	= $(LINUX_INC) \
	  $(TOP_DIR)/switch \
	  $(TOP_DIR)/switch/lisa \
	  $(TOP_DIR)/include

SRCS	= vlan_rename.c pktgen.c pktrecv.c test_sock.c mcast_recv.c mcast_send.c
SIMPLE_TARGETS = pktgen pktrecv test_sock mcast_recv mcast_send
TARGETS	= $(SIMPLE_TARGETS) vlan_rename

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

vlan_rename: vlan_rename.o
	$(Q) $(MAKE) -C $(TOP_DIR)/lib -C ../switch/lisa
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/shared.o ../lib/mm.o \
		../switch/lisa/lisa.o -lrt


$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

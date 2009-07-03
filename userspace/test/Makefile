TOP_DIR = ..

CGLAGS	= -g
INC	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include

SRCS	= pktgen.c pktrecv.c test_sock.c mcast_recv.c mcast_send.c
TARGETS	= $(SRCS:.c=)

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

$(TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

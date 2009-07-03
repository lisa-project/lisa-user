TOP_DIR = ..

CGLAGS	= -g
include $(TOP_DIR)/scripts/Makefile.includes
INC	= $(LINUX_INC) \
	  $(TOP_DIR)/include

SRCS	= pktgen.c pktrecv.c test_sock.c mcast_recv.c mcast_send.c
TARGETS	= $(SRCS:.c=)

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

$(TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

TOP_DIR = ..
FSM	= ./fsm

CFLAGS  = -fPIC -g
LDFLAGS = -lpthread -lm -lrt -L$(TOP_DIR)/switch -lswitch

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC 	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include \
	  .
DEF	= DEBUG

#SRCS	= listener.c rstpd.c rstp_utils.c bdm.c pim.c ppm.c prs.c prx.c pst.c ptx.c tcm.c
#SRCS	= listener.c rstpd.c rstp_utils.c $(FSM)/bdm.c $(FSM)/pim.c $(FSM)/ppm.c $(FSM)/prs.c $(FSM)/prx.c $(FSM)/pst.c $(FSM)/ptx.c $(FSM)/tcm.c
SRCS	= rstpd.c rstp_utils.c rstp_configuration.c rstp_receiver.c $(shell find $(FSM) -name '*.c')
TARGETS = rstp

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

LIB_OBJS = $(TOP_DIR)/lib/util.o \
	   $(TOP_DIR)/lib/mm.o \
	   $(TOP_DIR)/lib/if_generic.o \
	   $(TOP_DIR)/lib/netlink.o \
	   $(TOP_DIR)/lib/vector.o

$(TOP_DIR)/include/swsock.h:
	$(Q)$(MAKE) -C $(TOP_DIR)/include

$(LIB_OBJS): $(TOP_DIR)/lib/util.c $(TOP_DIR)/lib/if_generic.c
	$(Q)$(MAKE) -C $(TOP_DIR)/lib

rstp: $(TOP_DIR)/include/swsock.h $(LIB_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) -o $@ $(LIB_OBJS) $(OBJS) $(LDFLAGS)

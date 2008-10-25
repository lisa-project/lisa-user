TOP_DIR = ../
KDIR    = $(TOP_DIR)/../linux-2.6

CFLAGS  = -g
LDFLAGS = -L../cli -lswcli

INC = $(KDIR)/include \
      $(TOP_DIR)/include

SRCS    = swctl.c
TARGETS = swctl

all: swctl

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

swctl: $(OBJS)
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/misc.o

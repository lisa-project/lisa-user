TOP_DIR = ..

CFLAGS	=
LDFLAGS = -lcrypt
INC	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include

SRCS	= filter.c login.c reboot.c

TARGETS = filter login reboot

all: $(TARGETS) 

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

$(TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

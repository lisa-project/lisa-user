TOP_DIR = ../

LDFLAGS = -lcrypt

SRCS    = login.c
TARGETS	= login

all: login

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

login: $(OBJS)
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^

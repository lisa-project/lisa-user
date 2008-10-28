TOP_DIR		= ..
CFLAGS		= -fPIC -g
# LDFLAGS		= -L. -lswcli -lrt
INC		= $(TOP_DIR)/../linux-2.6/include \
		  $(TOP_DIR)/include
DEF		= _GNU_SOURCE CONFIG_CLI_TESTING

ifndef DIST
LOGIN_PATH	:= \"$(shell pwd)/swlogin\"
FILTER_PATH	:= \"$(shell pwd)/filter\"
DEF		+= LOGIN_PATH=$(LOGIN_PATH) FILTER_PATH=$(FILTER_PATH)
endif

ifdef USE_EXIT_IN_CONF
DEF		+= USE_EXIT_IN_CONF
endif

LIBSWCLI_SRCS = cli.c \
				rlshell.c
SRCS          = $(LIBSWCLI_SRCS) \
				swcli.c \
				test.c
LIBSWCLI_OBJS = $(LIBSWCLI_SRCS:.c=.o)

# Targets linked with libswcli
LIBSWCLI_TARGETS = swcli
# Simple targets, not linked with libswcli
SIMPLE_TARGETS	 = 
# All targets
TARGETS	= libswcli.so $(LIBSWCLI_TARGETS) $(SIMPLE_TARGETS)

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

libswcli.so: $(LIBSWCLI_OBJS)
	$(Q)$(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ -lreadline -lcurses -lutil -lcrypt -lrt $^ ../lib/misc.o ../lib/mm.o

$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) -o $@ $^

$(LIBSWCLI_TARGETS): % : %.o libswcli.so
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

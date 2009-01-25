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

# CLI Parser
LIBSWCLI_SRCS = cli.c

# Readline CLI Abstraction
LIBSWCLI_SRCS += rlshell.c

# LiSA Main Menu Tree
LIBSWCLI_SRCS += menu_main.c

# LiSA Configure Menu Tree
LIBSWCLI_SRCS += menu_config.c

# LiSA Configure-Interface Menu Tree
LIBSWCLI_SRCS += menu_config_if.c

# LiSA "clear mac-address-table" Menu Tree
LIBSWCLI_SRCS += menu_cl_mac_addr_t.c

# LiSA "show mac-address-table" Menu Tree
LIBSWCLI_SRCS += menu_sh_mac_addr_t.c

# LiSA Command Handlers
LIBSWCLI_SRCS += cmd_main.c

# LiSA Configure Command Handlers
LIBSWCLI_SRCS += cmd_config.c

# LiSA Interface Configuration Command Handlers
LIBSWCLI_SRCS += cmd_config_if.c

# Common swcli stuff that does not fit in the 4 layers
LIBSWCLI_SRCS += swcli_common.c

# Interface submenus
LIBSWCLI_SRCS += menu_interface.c

SRCS          = $(LIBSWCLI_SRCS) \
				swcli.c \
				filter.c \
				test.c
LIBSWCLI_OBJS = $(LIBSWCLI_SRCS:.c=.o)

# Targets linked with libswcli
LIBSWCLI_TARGETS = swcli test
# Simple targets, not linked with libswcli
SIMPLE_TARGETS	 = filter
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

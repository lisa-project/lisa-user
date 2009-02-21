TOP_DIR		:= $(shell pwd)/../
CFLAGS		= -fPIC -g
# LDFLAGS		= -L. -lswcli -lrt
INC		= $(TOP_DIR)/../linux-2.6/include \
		  $(TOP_DIR)/include \
		  $(TOP_DIR)/cli \
		  $(TOP_DIR)/cli/lib \
		  $(TOP_DIR)/cli/command
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
LIBSWCLI_SRCS += menu/main.c

# LiSA Configure Menu Tree
LIBSWCLI_SRCS += menu/config.c

# LiSA Configure-Interface Menu Tree
LIBSWCLI_SRCS += menu/config_if.c

# LiSA Configure-Line Menu Tree
LIBSWCLI_SRCS += menu/config_line.c

# LiSA Configure-Vlan Menu Tree
LIBSWCLI_SRCS += menu/config_vlan.c

# LiSA "clear mac-address-table" Menu Tree
LIBSWCLI_SRCS += menu/cl_mac_addr_t.c

# LiSA "show mac-address-table" Menu Tree
LIBSWCLI_SRCS += menu/sh_mac_addr_t.c

# LiSA Command Handlers
LIBSWCLI_SRCS += command/main.c

# LiSA Configure Command Handlers
LIBSWCLI_SRCS += command/config.c

# LiSA Interface Configuration Command Handlers
LIBSWCLI_SRCS += command/config_if.c

# LiSA Line Configuration Command Handlers
LIBSWCLI_SRCS += command/config_line.c

# LiSA Vlan Configuration Command Handlers
LIBSWCLI_SRCS += command/config_vlan.c

# Common swcli stuff that does not fit in the 4 layers
LIBSWCLI_SRCS += lib/common.c

# Interface submenus
LIBSWCLI_SRCS += lib/interface.c

SRCS          = $(LIBSWCLI_SRCS) \
				swcli.c \
				filter.c \
				list_commands.c
LIBSWCLI_OBJS = $(LIBSWCLI_SRCS:.c=.o)

# Targets linked with libswcli
LIBSWCLI_TARGETS = swcli list_commands
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

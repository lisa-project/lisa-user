TOP_DIR		:= $(shell pwd)/../
CFLAGS		= -fPIC -g
# LDFLAGS		= -L. -lswcli -lrt

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building the Command Line Interface *********)
endif

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC		= $(LINUX_INC) \
		  $(SWITCH_INC) \
		  $(TOP_DIR)/include \
		  $(TOP_DIR)/cli \
		  $(TOP_DIR)/cli/lib \
		  $(TOP_DIR)/cli/command \
		  $(TOP_DIR)/cli/cfgbuild
DEF		= _GNU_SOURCE CONFIG_CLI_TESTING

ifndef DIST
LOGIN_PATH	:= \"$(shell pwd)/swlogin\"
FILTER_PATH	:= \"$(shell pwd)/../tools/filter\"
DEF		+= LOGIN_PATH=$(LOGIN_PATH) FILTER_PATH=$(FILTER_PATH)
endif

# CLI Parser
LIBSWCLI_SRCS = cli.c

# Readline CLI Abstraction
LIBSWCLI_SRCS += rlshell.c

# LiSA Exec Menu Tree
LIBSWCLI_SRCS += menu/exec.c

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

# LiSA Exec Command Handlers
LIBSWCLI_SRCS += command/exec.c

# LiSA Configure Command Handlers
LIBSWCLI_SRCS += command/config.c

# LiSA Interface Configuration Command Handlers
LIBSWCLI_SRCS += command/config_if.c

# LiSA Line Configuration Command Handlers
LIBSWCLI_SRCS += command/config_line.c

# LiSA Vlan Configuration Command Handlers
LIBSWCLI_SRCS += command/config_vlan.c

# LiSA CDP Configuration Command Handlers
LIBSWCLI_SRCS += command/cdp.c

# Query LiSA Running Configuration and Build Config Commands
LIBSWCLI_SRCS += cfgbuild/cfgbuild.c

# Common swcli stuff that does not fit in the 4 layers
LIBSWCLI_SRCS += lib/swcli.c

# Swcli tokenizers
LIBSWCLI_SRCS += lib/tokenizers.c

# Interface submenus
LIBSWCLI_SRCS += lib/interface.c

SRCS          = $(LIBSWCLI_SRCS) \
				swcli.c \
				swcfgload.c \
				swlogin.c \
				swcon.c \
				list_commands.c
LIBSWCLI_OBJS = $(LIBSWCLI_SRCS:.c=.o)

# Targets linked with libswcli
LIBSWCLI_TARGETS = swcli swcfgload swlogin swcon list_commands

# All targets
TARGETS	= libswcli.so $(LIBSWCLI_TARGETS)

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

libswcli.so: $(LIBSWCLI_OBJS)
	$(Q)$(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ -lreadline -lcurses -lutil -lcrypt -lrt \
		$^ ../lib/util.o ../lib/mm.o ../lib/if_generic.o ../lib/netlink.o ../lib/cdp_client.o ../lib/rstp_client.o

$(LIBSWCLI_TARGETS): % : %.o libswcli.so $(TOP_DIR)/switch/libswitch.so
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

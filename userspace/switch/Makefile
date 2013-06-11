TOP_DIR = ..
CFLAGS  = -fPIC -g

include $(TOP_DIR)/scripts/Makefile.variables

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building [$(IMPL)] switch middleware *********)
endif

include $(TOP_DIR)/scripts/Makefile.includes

INC	=	$(TOP_DIR)/include \
		$(TOP_DIR)/switch

# Switch middleware initialization
LIBSWITCH_SRCS	= switch.c

# LiSA kernel module specific middleware implementation
ifeq ($(IMPL), lisa)
CFLAGS		+= -DLiSA
INC 		+=	$(LINUX_INC) \
				$(TOP_DIR)/switch/lisa
LIBSWITCH_SRCS	+= lisa/lisa.c
LIBSWITCH_SRCS	+= lisa/util_lisa.c
endif


# Linux bridge+8021q specific middleware implementation
ifeq ($(IMPL), linux)
CFLAGS 		+= -DLinux
INC 		+= $(TOP_DIR)/switch/linux
LIBSWITCH_SRCS	+= linux/linux.c
LIBSWITCH_SRCS	+= linux/util_linux.c
endif

# OpenWRT swconfig specific middleware implementation
ifeq ($(IMPL), openwrt)
CFLAGS 		+= -DOpenWRT
SWLIB_INC	:= $(TOP_DIR)/../../swconfig
INC 		+= $(TOP_DIR)/switch/swconfig $(SWLIB_INC)
LIBSWITCH_SRCS	+= swconfig/swconfig.c
LIBSWITCH_SRCS	+= swconfig/util_swlib.c
endif


SRCS		= $(LIBSWITCH_SRCS)

LIBSWITCH_OBJS	= $(LIBSWITCH_SRCS:.c=.o)

TARGETS = $(OBJS) libswitch.so

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

libswitch.so: $(LIBSWITCH_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ $^


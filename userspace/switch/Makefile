TOP_DIR = ..
CFLAGS  = -fPIC -g -I../include
LDFLAGS = -ldl -lm

include $(TOP_DIR)/scripts/Makefile.variables

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building [$(IMPL)] switch middleware *********)
endif

include $(TOP_DIR)/scripts/Makefile.includes

INC	=	$(TOP_DIR)/include \
		$(TOP_DIR)/switch

# Switch middleware initialization
LIBSWITCH_SRCS	= switch.c

# LiSA specific middleware implementation
ifeq ($(IMPL), lisa)
CFLAGS		+= -DLiSA
INC 		+=	$(LINUX_INC) \
			$(TOP_DIR)/switch/lisa
LIBSWITCH_SRCS	+= lisa/lisa.c
LIBSWITCH_SRCS	+= lisa/util_lisa.c
endif

ifeq ($(IMPL), linux)
CFLAGS 		+= -DLinux
INC 		+= $(TOP_DIR)/switch/linux
LIBSWITCH_SRCS	+= linux/linux.c
LIBSWITCH_SRCS	+= linux/util_linux.c

endif

SRCS		= $(LIBSWITCH_SRCS) multiengine.c

LIBSWITCH_OBJS	= $(LIBSWITCH_SRCS:.c=.o)

TARGETS = $(OBJS) libswitch.so multiengine

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

libswitch.so: $(LIBSWITCH_OBJS) $(OBJS) $(TOP_DIR)/lib/cJSON.o
	$(QUIET_LD)$(CC) $(CFLAGS) --export-dynamic -shared -Wl,-soname,$@ -o $@ $^ -lrt -ldl -lm

multiengine: multiengine.o
	$(QUIET_LD)$(CC) $(CFLAGS) -o $@ $^ $(TOP_DIR)/lib/cJSON.o $(LDFLAGS)



TOP_DIR = ..
CFLAGS  = -fPIC -g -I../include
LDFLAGS = -ldl -lm

include $(TOP_DIR)/scripts/Makefile.variables

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building [$(IMPL)] switch middleware *********)
endif

LIBSWITCH_NAME = "libswitch.so"

include $(TOP_DIR)/scripts/Makefile.includes

INC	=	$(TOP_DIR)/include \
		$(TOP_DIR)/switch

# Switch middleware initialization
LIBSWITCH_SRCS	= switch.c
MULTIENGINE_SRCS = cJSON.c multiengine.c

# LiSA specific middleware implementation
ifeq ($(IMPL), lisa)
LIBSWITCH_NAME   = liblisa.so
CFLAGS		+= -DLiSA
INC 		+=	$(LINUX_INC) \
			$(TOP_DIR)/switch/lisa
LIBSWITCH_SRCS	+= lisa/lisa.c
LIBSWITCH_SRCS	+= lisa/util_lisa.c
endif

ifeq ($(IMPL), linux)
LIBSWITCH_NAME   = liblinux.so
CFLAGS 		+= -DLinux
INC 		+= $(TOP_DIR)/switch/linux
LIBSWITCH_SRCS	+= linux/linux.c
LIBSWITCH_SRCS	+= linux/util_linux.c

endif

SRCS		= $(LIBSWITCH_SRCS)

LIBSWITCH_OBJS	= $(LIBSWITCH_SRCS:.c=.o)

TARGETS = $(OBJS) $(LIBSWITCH_NAME) multiengine.o

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

$(LIBSWITCH_NAME): $(LIBSWITCH_OBJS) $(OBJS) $(TOP_DIR)/lib/mm.o $(TOP_DIR)/lib/cJSON.o
	$(QUIET_LD)$(CC) $(CFLAGS) --export-dynamic -rdynamic -shared -Wl,-soname,$@ -o $@ $^ -lrt -ldl -lm

multiengine.o: multiengine.c
	$(QUIET_LD)$(CC) $(CFLAGS) -c -o $@  $^

multiengine: multiengine.o $(TOP_DIR)/lib/cJSON.o
	$(QUIET_LD)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

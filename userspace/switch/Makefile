TOP_DIR = ..

CFLAGS  = -fPIC -g

ifndef IMPL
	IMPL = lisa
	#IMPL = linux
endif

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building $(IMPL) switch middleware *********)
endif

ifeq ($(IMPL), lisa)
	SW_API_IMPL_DIR = lisa
	CFLAGS += -DLiSA
	LIB_OBJS = $(SW_API_IMPL_DIR)/util_lisa.o $(SW_API_IMPL_DIR)/lisa.o
endif

ifeq ($(IMPL), linux)
	SW_API_IMPL_DIR = linux
	CFLAGS += -DLinux
	LIB_OBJS = $(SW_API_IMPL_DIR)/util_linux.o $(SW_API_IMPL_DIR)/linux.o
endif

include $(TOP_DIR)/scripts/Makefile.includes
INC	= $(LINUX_INC) \
	  $(TOP_DIR)/include \
	  $(TOP_DIR)/switch/$(SW_API_IMPL_DIR) \
	  $(TOP_DIR)/switch

SRCS	= switch.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

TARGETS = $(OBJS) libswitch.so

.DEFAULT_GOAL = all

all: $(TARGETS)

libswitch.so: $(LIB_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ $^

dep: $(DEPS)

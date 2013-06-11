TOP_DIR		:= $(shell pwd)/../../
CFLAGS		+= -fPIC -g

SWLIB_INC	:= $(TOP_DIR)/../../swconfig

include $(TOP_DIR)/scripts/Makefile.includes
INC		= $(TOP_DIR)/switch \
		  $(TOP_DIR)/include \
		  $(SWLIB_INC)

# LiSA Middleware
SRCS = util_swlib.c swconfig.c
OBJS = $(SRCS:.c=.o)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

swconfig.o: $(OBJS) swconfig.h
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -lswlib $(OBJS)

dep: $(DEPS)

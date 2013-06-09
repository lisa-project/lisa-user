TOP_DIR		:= $(shell pwd)/../../
CFLAGS		+= -fPIC -g


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

util_swlib.o: $(OBJS) swconfig.h
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -lswlib $(OBJS)

dep: $(DEPS)

TOP_DIR		:= $(shell pwd)/../../
CFLAGS		+= -fPIC -g
LDFLAGS		+= -lm



include $(TOP_DIR)/scripts/Makefile.includes
INC		= $(LINUX_INC) \
		  $(TOP_DIR)/switch \
		  $(TOP_DIR)/include

# LiSA Middleware
SRCS = sw_api.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)

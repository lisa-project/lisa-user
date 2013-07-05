TOP_DIR		:= $(shell pwd)/../../
CFLAGS		+= -fPIC -g



include $(TOP_DIR)/scripts/Makefile.includes
INC		= $(TOP_DIR)/switch \
INC		= $(TOP_DIR)/switch_api \
		  $(TOP_DIR)/include

# LiSA Middleware
SRCS = sw_api.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)

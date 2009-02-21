TOP_DIR = ..

CFLAGS  = -fPIC -g
INC	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include

SRCS	= mm.c util.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all
all: $(OBJS)

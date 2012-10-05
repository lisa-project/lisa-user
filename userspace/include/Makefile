TOP_DIR = ..

TARGETS = swsock.h

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: all

swsock.h: $(LINUX_INC)/linux/socket.h
	$(Q)../../mkheaders.sh swsock.h $(LINUX_INC) > swsock.h

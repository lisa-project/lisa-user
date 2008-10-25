TOP_DIR = ..

TARGETS = swsock.h

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

swsock.h: $(TOP_DIR)/../linux-2.6/include/linux/socket.h
	$(Q)../../mkheaders.sh swsock.h > swsock.h

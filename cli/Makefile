CFLAGS = -g -Wall #-DDEBUG
LDFLAGS = -lreadline -lcurses -lutil

# FIXME
__LOGIN_PATH := \"$(shell pwd)/swlogin\"
CFLAGS += -DLOGIN_PATH=$(__LOGIN_PATH)

.PHONY: all clean

all: swcli swclid swlogin filter

swcli: swcli.o climain.o command.o config.o

swclid: swclid.o

filter: filter.o

swlogin: swlogin.o climain.o command.o config.o

clean:
	rm -f *.o *~ swcli swclid swlogin filter

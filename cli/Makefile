CFLAGS = -g -DDEBUG
LDFLAGS = -lefence -lreadline -lcurses -lutil

# FIXME
__LOGIN_PATH := \"$(shell pwd)/swlogin\"
CFLAGS += -DLOGIN_PATH=$(__LOGIN_PATH)

.PHONY: all clean

all: swcli swclid swlogin

swcli: swcli.o climain.o

swclid: swclid.o

swlogin: swlogin.o climain.o

clean:
	rm -f *.o *~ swcli swclid swlogin

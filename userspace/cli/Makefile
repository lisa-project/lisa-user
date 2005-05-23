CFLAGS = -g -Wall -I../../linux-2.6.10/include #-DDEBUG
LDFLAGS = -lreadline -lcurses -lutil -lcrypt

# FIXME
__LOGIN_PATH := \"$(shell pwd)/swlogin\"
CFLAGS += -DLOGIN_PATH=$(__LOGIN_PATH)

.PHONY: all clean

all: swcli swclid swlogin filter

swcli: swcli.o climain.o command.o config.o config_if.o build_config.o if.o shared.o netlink.o ip.o
	cd ../lib && make
	$(CC) $(LDFLAGS) -o $@ $^ ../lib/misc.o

swclid: swclid.o shared.o

filter: filter.o

swlogin: swlogin.o climain.o command.o config.o config_if.o build_config.o if.o shared.o netlink.o ip.o
	cd ../lib && make
	$(CC) $(LDFLAGS) -o $@ $^ ../lib/misc.o

clean:
	rm -f *.o *~ swcli swclid swlogin filter

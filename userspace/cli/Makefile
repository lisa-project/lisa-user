CFLAGS = -g -Wall -I../../linux-2.6.10/include #-DDEBUG
LDFLAGS = -L. -lreadline -lcurses -lutil -lcrypt

# FIXME
__LOGIN_PATH := \"$(shell pwd)/swlogin\"
CFLAGS += -DLOGIN_PATH=$(__LOGIN_PATH)

.PHONY: all clean

all: swcli swclid swlogin filter swcon swcfgload

libswcli.so: climain.o command.o config.o config_if.o config_line.o build_config.o if.o shared.o netlink.o ip.o
	cd ../lib && make
	$(CC) -s -shared -Wl,-soname,libswcli.so -o $@ $^ ../lib/misc.o

swcli: swcli.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

swclid: swclid.o shared.o

swcon: swcon.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

swcfgload: swcfgload.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

filter: filter.o

swlogin: swlogin.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

clean:
	rm -f *.o *~ libswcli.so swcli swclid swlogin filter swcon

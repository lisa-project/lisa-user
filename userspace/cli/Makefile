CFLAGS = -g -Wall -I$(KDIR)/include -I ../include -fPIC #-DDEBUG
LDFLAGS = -L. -lreadline -lcurses -lutil -lcrypt

__LOGIN_PATH := \"$(shell pwd)/swlogin\"
__FILTER_PATH := \"$(shell pwd)/filter\"

ifndef DIST
CFLAGS += -DLOGIN_PATH=$(__LOGIN_PATH) -DFILTER_PATH=$(__FILTER_PATH)
endif

ifdef USE_EXIT_IN_CONF
CFLAGS += -DUSE_EXIT_IN_CONF
endif

.PHONY: all clean

all: swcli swclid swlogin filter swcon swcfgload reboot

libswcli.so: climain.o command.o config.o config_if.o config_line.o build_config.o if.o shared.o netlink.o ip.o
	cd ../lib && make
	$(CC) $(CFLAGS) -shared -Wl,-soname,libswcli.so -o $@ $^ ../lib/misc.o

swcli: swcli.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

swclid: swclid.o shared.o

swcon: swcon.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

swcfgload: swcfgload.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

filter: filter.o

reboot: reboot.o

swlogin: swlogin.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

clean:
	rm -f *.o *~ libswcli.so swcli swclid swlogin filter swcon swcfgload reboot

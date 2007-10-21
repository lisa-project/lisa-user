CFLAGS = -g -Wall -I$(KDIR)/include -I ../include -fPIC -D_GNU_SOURCE
LDFLAGS = -L. -lreadline -lcurses -lutil -lcrypt -lrt

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

libswcli.so: climain.o command.o config.o config_if.o config_line.o build_config.o if.o shared.o netlink.o ip.o cdp.o ../lib/misc.o ../lib/mm.o
	cd ../lib && make
	$(CC) $(CFLAGS) -shared -Wl,-soname,libswcli.so -o $@ $^

swcli: swcli.o libswcli.so
	$(CC) $(LDFLAGS) -lswcli -o $@ $^

swclid: swclid.o shared.o ../lib/mm.o

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

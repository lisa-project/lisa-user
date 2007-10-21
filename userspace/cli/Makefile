CFLAGS = -g -Wall -I$(KDIR)/include -I ../include -fPIC -D_GNU_SOURCE
LDFLAGS = -L. -lswcli

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

libswcli.so: climain.o command.o config.o config_if.o config_line.o build_config.o if.o shared.o netlink.o ip.o cdp.o
	cd ../lib && make
	$(CC) $(CFLAGS) -shared -Wl,-soname,libswcli.so -o $@ -lreadline -lcurses -lutil -lcrypt -lrt $^ ../lib/misc.o ../lib/mm.o

swcli: swcli.o libswcli.so

swclid: swclid.o libswcli.so

swcon: swcon.o libswcli.so

swcfgload: swcfgload.o libswcli.so

swlogin: swlogin.o libswcli.so

clean:
	rm -f *.o *~ libswcli.so swcli swclid swlogin filter swcon swcfgload reboot

# Things that don't need to be linked against libswcli.so (and thus we
# explicitly define the build command to avoid using LDFLAGS)

filter: filter.o
	$(CC) $(CFLAGS) -o $@ $^

reboot: reboot.o
	$(CC) $(CFLAGS) -o $@ $^

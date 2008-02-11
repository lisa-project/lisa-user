KDIR := $(shell readlink -f ../linux-2.6)
export KDIR

.PHONY: all clean install dep

all:
	cd cli && make
	cd login && make
	cd swctl && make
	cd cdpd && make

dep:
	cd cli && make dep
	cd login && make dep
	cd swctl && make dep
	cd cdpd && make dep

clean:
	cd cli && make clean
	cd lib && make clean
	cd login && make clean
	cd swctl && make clean
	cd cdpd && make clean

install: all
	install -m 755 -D cdpd/cdpd $(ROOT)/usr/sbin/cdpd
	install -m 755 -D cli/filter $(ROOT)/bin/filter
ifneq ($(ROOT),)
	install -m 755 -D cli/filter $(ROOT)/bin/reboot
endif
	install -m 755 -D cli/swcfgload $(ROOT)/usr/sbin/swcfgload
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swcli
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swclid
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swcon
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swlogin
	install -m 755 -D cli/libswcli.so $(ROOT)/usr/lib/libswcli.so
ifneq ($(ROOT),)
	install -m 755 -D login/login $(ROOT)/bin/login
endif
	install -m 755 -D swctl/swctl $(ROOT)/usr/sbin/swctl
ifeq ($(ROOT),)
	/sbin/ldconfig
endif

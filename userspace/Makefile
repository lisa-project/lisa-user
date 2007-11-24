KDIR := $(shell readlink -f ../linux-2.6)
export KDIR

all:
	cd cli && make
	cd login && make
	cd swctl && make
	cd cdpd && make

clean:
	cd cli && make clean
	cd lib && make clean
	cd login && make clean
	cd swctl && make clean
	cd cdpd && make clean

install:
	install -m 755 cli/swcli /usr/sbin/swcli
	install -m 755 cli/filter /bin/filter
	install -m 755 cli/swcfgload /usr/sbin/swcfgload
	install -m 755 swctl/swctl /usr/sbin/swctl
	install -m 755 cdpd/cdpd /usr/sbin/cdpd
	install -m 755 cli/libswcli.so /usr/lib/libswcli.so
	/sbin/ldconfig

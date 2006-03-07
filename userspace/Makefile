KDIR := $(shell readlink -f ../linux-2.6.15)
export KDIR

all:
	cd cli && make
	cd login && make
	cd swctl && make

clean:
	cd cli && make clean
	cd lib && make clean
	cd login && make clean
	cd swctl && make clean

.PHONY: all clean

CFLAGS += -g -Wall -I$(KDIR)/include -I../include -fPIC -DDEBUG $(shell libnet-config --cflags --defines)
LDFLAGS=-lpcap $(shell libnet-config --libs)


all: cdpd

cdp: cdpd.c

clean:
	rm -f *.o cdpd

.PHONY: all clean

CFLAGS += -g -Wall -I$(KDIR)/include -I../include -fPIC -D_XOPEN_SOURCE -DDEBUG $(shell libnet-config --cflags --defines)
LDFLAGS=-lpcap -lpthread -lefence $(shell libnet-config --libs)


all: cdpd

cdpd: cdp_aging.o cdp_configuration.o cdp_send.o cdpd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o cdpd

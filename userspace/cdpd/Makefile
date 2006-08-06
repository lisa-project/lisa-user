.PHONY: all clean

CFLAGS +=-Wall -I$(KDIR)/include -I../include -fPIC -D_XOPEN_SOURCE=500 $(shell libnet-config --cflags --defines)
#CFLAGS +=-DDEBUG
LDFLAGS=-lpcap -lpthread -lm $(shell libnet-config --libs)

all: cdpd

cdpd: cdp_aging.o cdp_configuration.o cdp_send.o cdpd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o cdpd

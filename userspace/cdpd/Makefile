.PHONY: all clean

CFLAGS +=-Wall -I$(KDIR)/include -I../include -fPIC -D_XOPEN_SOURCE=500
CFLAGS +=-DDEBUG
CFLAGS +=-g
LDFLAGS=-lpthread -lm -lrt

all: cdpd test_sock

cdpd: cdp_aging.o cdp_configuration.o cdp_send.o cdpd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o cdpd test_sock

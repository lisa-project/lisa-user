CFLAGS = -I$(KDIR)/include -I ../include -Wall
LDFLAGS = -L../cli -lswcli

.PHONY: all clean

all: swctl

swctl: swctl.o
	cd ../lib && make
	$(CC) $(LDFLAGS) -o swctl $^ ../lib/misc.o

clean:
	rm -f *.o *~ swctl

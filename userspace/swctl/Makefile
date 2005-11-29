CFLAGS = -I$(KDIR)/include -Wall
LDFLAGS =

.PHONY: all clean

all: swctl

swctl: swctl.o
	cd ../lib && make
	$(CC) $(LDFLAGS) -o swctl $^ ../lib/misc.o

clean:
	rm -f *.o *~ swctl

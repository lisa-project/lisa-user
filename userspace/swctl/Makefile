CFLAGS = -I$(KDIR)/include -I ../include -Wall -g
LDFLAGS = -L../cli -lswcli

.PHONY: all clean dep

all: swctl

swctl: swctl.o
	cd ../lib && make
	$(CC) $(LDFLAGS) -o swctl $^ ../lib/misc.o

clean:
	rm -f *.o *~ swctl

dep:
	$(CC) $(CFLAGS) -MM *.c >> $(DEP)

CFLAGS = -I../linux-2.6.10/include -Wall
LDFLAGS =

.PHONY: all clean

all: swctl

swctl: swctl.o
	cd ../lib && make
	$(CC) -o swctl $< ../lib/misc.o

clean:
	rm -f *.o *~ swctl

CFLAGS = -g
LDFLAGS = -lefence -lreadline -lcurses

.PHONY: all clean

all: swcli swclid

swcli: swcli.o climain.o

swclid: swclid.o climain.o

clean:
	rm -f *.o *~ swcli swclid

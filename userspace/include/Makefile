TARGETS := swsock.h

all: $(TARGETS)

.PHONY: all clean

swsock.h: ../../linux-2.6/include/linux/socket.h
	../../mkheaders.sh swsock.h > swsock.h

clean:
	rm -f $(TARGETS)

.PHONY: all clean dep

LDFLAGS = -lcrypt

all: login

clean:
	rm -f *~ *.o login

dep:
	$(CC) $(CFLAGS) -MM *.c >> $(DEP)

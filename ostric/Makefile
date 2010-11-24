include ../makeflags

all: ostric

ostric: ostric.o escape.o cmds.o
	$(CC) $(LDFLAGS) -o $@ $^

escape.o: ../sricd/escape.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean

clean:
	-rm -f *.o ostric

include ../makeflags

all: ostric

ostric: ostric.o
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean

clean:
	-rm -f *.o ostric

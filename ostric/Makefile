include ../makeflags

CFLAGS+=	`pkg-config --cflags glib-2.0`
LDFLAGS+=	`pkg-config --libs glib-2.0`

all: ostric

ostric: ostric.o escape.o cmds.o crc16.o generic.o reply.o
	$(CC) $(LDFLAGS) -o $@ $^

escape.o: ../sricd/escape.c
	$(CC) $(CFLAGS) -c -o $@ $^

crc16.o:
	cd ../sricd/crc16 ; make
	cp ../sricd/crc16/crc16.o .

.PHONY: clean

clean:
	-rm -f *.o ostric

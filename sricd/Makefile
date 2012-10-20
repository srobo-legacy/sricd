include ../makeflags

all: sricd

include depend

CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

depend: *.c
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o output-queue.o sricd.o ipc.o log.o escape.o \
	device.o sric-if.o crc16/crc16.o sric-enum.o
	$(CC) -o $@ $^ $(LDFLAGS)

crc16/crc16.o:
	cd crc16 ; make

.PHONY: clean

clean:
	rm -f *.o sricd depend
	cd crc16 ; make clean

include ../makeflags

all: sricd

include depend

CFLAGS += -I../poolalloc

depend: *.c
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o input.o output-queue.o queue.o sricd.o ipc.o log.o sched.o escape.o device.o sric-if.o ../poolalloc/pool.o crc16/crc16.o
	$(CC) $(LDFLAGS) -o $@ $^

crc16/crc16.o:
	cd crc16 ; make

.PHONY: clean

clean:
	rm -f *.o sricd depend
	cd crc16 ; make clean

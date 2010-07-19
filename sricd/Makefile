all: sricd

include depend

CFLAGS += -I../poolalloc

depend: *.c
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o input.o output-queue.o queue.o sricd.o ipc.o log.o sched.o hashtable.o device.o ../poolalloc/pool.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o sricd depend

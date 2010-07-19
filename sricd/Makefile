all: sricd

include depend

depend:
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o input.o output-queue.o pool.o queue.o sricd.o ipc.o log.o sched.o hashtable.c
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o sricd depend

all: sricd

include depend

depend:
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o input.o output-queue.o pool.o queue.o sricd.o
	$(CC) $(LDFLAGS) -o $@ $^

client.o: client.c
	$(CC) -c $(CFLAGS) -o $@ $<

init.o: init.c
	$(CC) -c $(CFLAGS) -o $@ $<

input.o: input.c
	$(CC) -c $(CFLAGS) -o $@ $<

output-queue.o: output-queue.c
	$(CC) -c $(CFLAGS) -o $@ $<

pool.o: pool.c
	$(CC) -c $(CFLAGS) -o $@ $<

queue.o: queue.c
	$(CC) -c $(CFLAGS) -o $@ $<

sricd.o: sricd.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o sricd depend

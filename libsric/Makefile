all: libsric.a

include depend

depend:
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

sricd: client.o init.o input.o output-queue.o pool.o queue.o sricd.o
	$(CC) $(LDFLAGS) -o $@ $^

libsric.a: sric.o
	rm -f $@
	ar -cqs $@ $^

clean:
	rm -f *.o libsric.a depend

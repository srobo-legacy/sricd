include ../makeflags

all: libsric.a libsric.so

include depend

depend: *.c
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

libsric.a: sric.o
	rm -f $@
	ar -cqs $@ $^

libsric.so: sric.o
	$(CC) $^ -o $@ -shared

.PHONY: clean

clean:
	rm -f *.o libsric.a depend

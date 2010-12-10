include ../makeflags

all: libsric.a pysricmodule.so

include depend

depend: *.c
	rm -f depend
	for file in *.c; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

libsric.a: sric.o
	rm -f $@
	ar -cqs $@ $^

# XXX - python doesn't appear to have pkgconfig?
pysricmodule.so: pysric.c
	$(CC) $(LDFLAGS) -I/usr/include/python2.6 $^ -o $@ -shared

.PHONY: clean

clean:
	rm -f *.o libsric.a pysricmodule.so depend

include makeflags

all: libsric/libsric.a sricd/sricd poolalloc/pool.o

install: all
	install -d $(prefix)/bin
	install -d $(prefix)/include
	install -d $(prefix)/lib
	install -s sricd/sricd $(prefix)/bin/sricd]
	install -m 0644 libsric/sric.h $(prefix)/include/sric.h
	install -m 0644 libsric/libsric.a $(prefix)/lib/libsric.a

libsric/libsric.a:
	cd libsric ; make

sricd/sricd: poolalloc/pool.o
	cd sricd ; make

poolalloc/pool.o:
	cd poolalloc ; make

.PHONY: clean

clean:
	cd libsric ; make clean
	cd sricd ; make clean
	cd poolalloc ; make clean

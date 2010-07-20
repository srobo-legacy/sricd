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
	$(MAKE) -C libsric

sricd/sricd: poolalloc/pool.o
	$(MAKE) -C sricd

poolalloc/pool.o:
	$(MAKE) -C poolalloc

.PHONY: clean

clean:
	$(MAKE) -C libsric clean
	$(MAKE) -C sricd clean
	$(MAKE) -C poolalloc clean

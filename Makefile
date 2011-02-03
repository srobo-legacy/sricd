include makeflags

all: libsric/libsric.a sricd/sricd

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/lib
	install sricd/sricd $(DESTDIR)$(PREFIX)/bin/sricd
	install -m 0644 libsric/sric.h $(DESTDIR)$(PREFIX)/include/sric.h
	install -m 0644 libsric/libsric.a $(DESTDIR)$(PREFIX)/lib/libsric.a
	install -m 0644 libsric/libsric.so $(DESTDIR)$(PREFIX)/lib/libsric.so

libsric/libsric.a:
	$(MAKE) -C libsric

sricd/sricd:
	$(MAKE) -C sricd

.PHONY: clean

clean:
	$(MAKE) -C libsric clean
	$(MAKE) -C sricd clean

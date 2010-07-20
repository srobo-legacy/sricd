crc16.o: crc16.c crc16.h

crc16.c: crc16-build
	./crc16-build > $@

crc16-build: crc16-build.o

crc16-build.o: crc16-build.c

.PHONY: clean

clean:
	rm -f crc16.c *.o crc16-build

include ../makeflags

CFLAGS +=	`pkg-config --cflags glib-2.0`
LDFLAGS +=	`pkg-config --libs glib-2.0`

all: trumpets

trumpets: trumpets.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	-rm *.o trumpets

.PHONY: clean

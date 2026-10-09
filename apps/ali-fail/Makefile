CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-fail

ali-fail: src/main.c
	$(CC) $(CFLAGS) -o ali-fail src/main.c $(LDFLAGS)

clean:
	rm -f ali-fail *.o

install: ali-fail
	install -Dm755 ali-fail $(DESTDIR)/usr/bin/ali-fail
	install -Dm755 ali-fail-screen $(DESTDIR)/usr/bin/ali-fail-screen
	install -Dm644 ali-fail@.service $(DESTDIR)/usr/lib/systemd/system/ali-fail@.service

.PHONY: all clean install

CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-hymns

ali-hymns: src/main.c
	$(CC) $(CFLAGS) -o ali-hymns src/main.c $(LDFLAGS)

clean:
	rm -f ali-hymns *.o

install: ali-hymns
	install -Dm755 ali-hymns $(DESTDIR)/usr/bin/ali-hymns
	install -Dm644 ali-hymns.desktop $(DESTDIR)/usr/share/applications/ali-hymns.desktop
	install -Dm644 ../ali-style/icons/ali-hymns.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/ali-hymns.svg

.PHONY: all clean install

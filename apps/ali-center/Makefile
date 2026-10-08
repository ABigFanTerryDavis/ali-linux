CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-center

ali-center: src/main.c
	$(CC) $(CFLAGS) -o ali-center src/main.c $(LDFLAGS)

clean:
	rm -f ali-center *.o

install: ali-center
	install -Dm755 ali-center $(DESTDIR)/usr/bin/ali-center
	install -Dm644 ali-center.desktop $(DESTDIR)/usr/share/applications/ali-center.desktop
	install -Dm644 ali-center-tr.desktop $(DESTDIR)/usr/share/applications/ali-center-tr.desktop
	install -Dm644 ../ali-style/icons/ali-center.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/ali-center.svg

.PHONY: all clean install

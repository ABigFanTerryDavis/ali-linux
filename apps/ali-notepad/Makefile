CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0 gtksourceview-5)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0 gtksourceview-5)

all: ali-notepad

ali-notepad: src/main.c
	$(CC) $(CFLAGS) -o ali-notepad src/main.c $(LDFLAGS)

clean:
	rm -f ali-notepad *.o

install: ali-notepad
	install -Dm755 ali-notepad $(DESTDIR)/usr/bin/ali-notepad
	install -Dm644 ali-notepad.desktop $(DESTDIR)/usr/share/applications/ali-notepad.desktop
	install -Dm644 ../ali-style/icons/ali-notepad.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/ali-notepad.svg

.PHONY: all clean install

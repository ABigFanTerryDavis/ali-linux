CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-packages

ali-packages: src/main.c
	$(CC) $(CFLAGS) -o ali-packages src/main.c $(LDFLAGS)

clean:
	rm -f ali-packages *.o

install: ali-packages
	install -Dm755 ali-packages $(DESTDIR)/usr/bin/ali-packages
	install -Dm644 ali-packages.desktop $(DESTDIR)/usr/share/applications/ali-packages.desktop
	install -Dm644 ali-packages-tr.desktop $(DESTDIR)/usr/share/applications/ali-packages-tr.desktop

.PHONY: all clean install

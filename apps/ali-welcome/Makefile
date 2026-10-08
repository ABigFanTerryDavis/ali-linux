CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-welcome

ali-welcome: src/main.c
	$(CC) $(CFLAGS) -o ali-welcome src/main.c $(LDFLAGS)

clean:
	rm -f ali-welcome *.o

install: ali-welcome
	install -Dm755 ali-welcome $(DESTDIR)/usr/bin/ali-welcome
	install -Dm644 ali-welcome.desktop $(DESTDIR)/usr/share/applications/ali-welcome.desktop

.PHONY: all clean install

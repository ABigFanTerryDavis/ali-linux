CC=gcc
CFLAGS=-Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

all: ali-shrine

ali-shrine: src/main.c
	$(CC) $(CFLAGS) -o ali-shrine src/main.c $(LDFLAGS)

clean:
	rm -f ali-shrine *.o

install: ali-shrine
	install -Dm755 ali-shrine $(DESTDIR)/usr/bin/ali-shrine

.PHONY: all clean install

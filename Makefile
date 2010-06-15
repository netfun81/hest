DEBUG   = -O3
DEBUG   = -O0 -ggdb -g3
CFLAGS  = -Wall -pedantic -ansi -std=c99 $(DEBUG)
LDFLAGS = -lX11
BINARY  = hest
VERSION = 0.1
PREFIX  = /usr/local
OBJECTS = $(BINARY).o

all: $(BINARY)

clean:
	rm -f $(BINARY).o $(BINARY)

install: all
	mkdir -p $(PREFIX)/bin
	cp -f $(BINARY) $(PREFIX)/bin/$(BINARY)
	sed "s/VERSION/$(VERSION)/g" < $(BINARY).1 > $(PREFIX)/man/man1/$(BINARY).1

uninstall:
	rm -f $(BINARY) $(PREFIX)/bin/$(BINARY)

VERSION = 0.2

CFLAGS  = -Wall -Wextra -pedantic -std=c99 -O3
LDFLAGS = -lX11 -lxinerama

PREFIX  = /usr/local
BINARY  = hest
BINPATH = $(PREFIX)/bin/

MANSECT = 1
MANPAGE = $(BINARY).$(MANSECT)
MANBASE = $(PREFIX)/share/man/man$(MANSECT)
MANPATH = $(MANBASE)/$(MANPAGE)

$(BINARY): $(BINARY).o

all: $(BINARY)

clean:
        rm -f $(BINARY) *.o

install: all
        install -Ds $(BINARY) $(BINPATH)
        install -D -m 644 $(MANPAGE) $(MANPATH)
        sed -i "s/VERSION/$(VERSION)/g" $(MANPATH)

uninstall:
        rm -f $(BINPATH)
        rm -f $(MANPATH)

.PHONY: all clean install uninstall

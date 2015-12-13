TARGET = networktablet
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CFLAGS=-g

networktablet : networktablet.c

clean :
	rm -f networktablet

install :
	install -D networktablet $(DESTDIR)$(BINDIR)/networktablet

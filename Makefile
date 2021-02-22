LANGUAGE = po/strings_pt.c
CFLAGS = -g -Og -pipe -Ipo/ -D_GNU_SOURCE

# defaults to cproc because "why not?"
CC = cproc

PREFIX = ${HOME}/.local
bindir = $(PREFIX)/bin

all: ep

ep: ep.c out.c path.c git.c $(LANGUAGE)

install: ep
	install -m755 $< $(bindir)/ep

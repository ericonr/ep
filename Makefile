LANGUAGE = PORTUGUESE
CFLAGS = -g -Og -pipe -Ipo/ -D$(LANGUAGE)

# defaults to cproc because "why not?"
CC = cproc

PREFIX = ${HOME}/.local
bindir = $(PREFIX)/bin

all: ep

ep: ep.c git.c

install: ep
	install -m755 $< $(bindir)/ep

LANGUAGE = po/strings_pt.c
CFLAGS = -g -Og -pipe -Ipo/ -D_GNU_SOURCE

# default to static to run on glibc and musl
STATIC = -static
LDFLAGS = $(STATIC) -pthread

# defaults to cproc because "why not?"
CC = cproc

PREFIX = ${HOME}/.local
bindir = $(PREFIX)/bin

all: ep

ep: ep.c out.c path.c git.c lang.c ssh.c $(LANGUAGE)

install: ep
	install -m755 $< $(bindir)/ep

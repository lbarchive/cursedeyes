PACKAGE ?= cursedeyes
VERSION ?= $(shell hg log --template '{latesttag}-{latesttagdistance}-{node|short} ({date|rfc3339date})' -r . || echo unknown)

PROGRAM = cursedeyes
SOURCES = cursedeyes.c

CC       ?= gcc
CFLAGS   ?= -g -O2
CFLAGS   += -std=c99 -Wall -Wextra -pedantic
CFLAGS   += $(shell pkg-config ncursesw --cflags)
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDLIBS   += -lm $(shell pkg-config ncursesw --libs)

INSTALL     = install
INSTALL_BIN = $(INSTALL) -D -m 755

PREFIX   = /usr/local

bin_dir  = $(PREFIX)/bin

CPPFLAGS += -DPREFIX=$(PREFIX)   \
            -DPACKAGE=$(PACKAGE) \
            -DPROGRAM=$(PROGRAM) \
            -DVERSION='$(VERSION)'

all: $(PROGRAM)
.PHONY: all

$(PROGRAM): $(SOURCES)

clean:
	$(RM) *.o
.PHONY: clean

clobber: clean
	$(RM) $(PROGRAM)
.PHONY: clobber

install: $(PROGRAM)
	$(INSTALL_BIN) $(PROGRAM) $(DESTDIR)$(bin_dir)/$(PROGRAM)
.PHONY: install

install-strip:
	$(MAKE) INSTALL_BIN='$(INSTALL_BIN) -s' install
.PHONY: install-strip

uninstall:
	$(RM) $(DESTDIR)$(bin_dir)/$(PROGRAM)
.PHONY: uninstall

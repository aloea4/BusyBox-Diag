CC := gcc
OPT ?= -O3 -flto
CFLAGS := -Wall -Wextra -std=c99 $(OPT) -I../../libdiag/include
LIBDIAG_DIR := ../../libdiag
LIBDIAG_A := $(LIBDIAG_DIR)/libdiag.a
LIBDIAG_CFLAGS := -Wall -Wextra -std=c99 $(OPT) -Iinclude
PREFIX ?= /usr/local
MANDIR ?= $(PREFIX)/share/man/man1

OBJS := diagnet.o \
        diagnet_state.o \
        diagnet_collect.o \
        diagnet_filter.o \
        diagnet_analyze.o \
        diagnet_policy.o \
        diagnet_output.o

.PHONY: all clean libdiag install-man uninstall-man

all: diagnet

libdiag $(LIBDIAG_A):
	$(MAKE) -C $(LIBDIAG_DIR) CFLAGS="$(LIBDIAG_CFLAGS)"

diagnet: $(OBJS) $(LIBDIAG_A)
	$(CC) $(CFLAGS) $(OBJS) $(LIBDIAG_A) -o $@

%.o: %.c diagnet.h
	$(CC) $(CFLAGS) -c $< -o $@

install-man: diagnet.1
	install -d $(DESTDIR)$(MANDIR)
	install -m 0644 diagnet.1 $(DESTDIR)$(MANDIR)/

uninstall-man:
	rm -f $(DESTDIR)$(MANDIR)/diagnet.1

clean:
	rm -f diagnet $(OBJS)
	$(MAKE) -C $(LIBDIAG_DIR) clean

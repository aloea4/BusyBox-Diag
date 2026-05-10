CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -I../../libdiag/include
LIBDIAG_DIR := ../../libdiag
LIBDIAG_A := $(LIBDIAG_DIR)/libdiag.a

.PHONY: all clean libdiag

all: diagnet

libdiag $(LIBDIAG_A):
	$(MAKE) -C $(LIBDIAG_DIR)

diagnet: diagnet.c $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f diagnet

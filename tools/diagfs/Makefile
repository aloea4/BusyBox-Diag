CC := gcc

OPT ?= -O3 -flto
CFLAGS := -Wall -Wextra -std=c99 $(OPT) -DDIAGFS_STANDALONE -I../../libdiag/include
LIBDIAG_DIR := ../../libdiag
LIBDIAG_A := $(LIBDIAG_DIR)/libdiag.a
LIBDIAG_CFLAGS := -Wall -Wextra -std=c99 $(OPT) -Iinclude

SRCS := diagfs.c diagfs_analyze.c diagfs_policy.c diagfs_output.c diagfs_mount.c

.PHONY: all clean libdiag

all: diagfs

libdiag $(LIBDIAG_A):
	$(MAKE) -C $(LIBDIAG_DIR) CFLAGS="$(LIBDIAG_CFLAGS)"

diagfs: $(SRCS) $(LIBDIAG_A)
	$(CC) $(CFLAGS) $(SRCS) $(LIBDIAG_A) -o $@

clean:
	rm -f diagfs
	$(MAKE) -C $(LIBDIAG_DIR) clean
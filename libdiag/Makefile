CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -Iinclude
AR := ar
ARFLAGS := rcs

LIBDIAG_SRC := \
	src/libdiag/diag_reader.c \
	src/libdiag/diag_parse.c \
	src/libdiag/diag_proc.c \
	src/libdiag/diag_fs.c \
	src/libdiag/diag_net.c

LIBDIAG_OBJ := $(LIBDIAG_SRC:.c=.o)
LIBDIAG_A := libdiag.a

EXAMPLES := \
	examples/test_proc \
	examples/test_fs \
	examples/test_net \
	examples/test_parse \
	examples/test_cpu \
	examples/test_fiemap \
	examples/test_errors \
	examples/test_fixtures

.PHONY: all clean test

all: $(LIBDIAG_A) $(EXAMPLES)

$(LIBDIAG_A): $(LIBDIAG_OBJ)
	$(AR) $(ARFLAGS) $@ $^

examples/test_proc: examples/test_proc.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_fs: examples/test_fs.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_net: examples/test_net.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_parse: examples/test_parse.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_cpu: examples/test_cpu.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@
examples/test_fiemap: examples/test_fiemap.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_errors: examples/test_errors.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

examples/test_fixtures: examples/test_fixtures.o $(LIBDIAG_A)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	./examples/test_proc
	./examples/test_fs
	./examples/test_net
	./examples/test_parse
	./examples/test_cpu
	./examples/test_fiemap
	./examples/test_errors
	./examples/test_fixtures
clean:
	rm -f $(LIBDIAG_OBJ) examples/*.o $(LIBDIAG_A) $(EXAMPLES)
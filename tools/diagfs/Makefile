CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -I../../libdiag/include
LDFLAGS = ../../libdiag/libdiag.a

TARGET  = diagfs
SRC     = diagfs.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
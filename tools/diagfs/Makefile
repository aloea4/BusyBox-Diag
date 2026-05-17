CC      = gcc
# 將 -O2 和 -DDIAGFS_STANDALONE 加進編譯參數中
CFLAGS = -O3 -flto -DDIAGFS_STANDALONE -Wall -Wextra -std=c99 -I../../libdiag/include
LIBS    = ../../libdiag/libdiag.a

SRCS = diagfs.c diagfs_analyze.c diagfs_policy.c diagfs_output.c diagfs_mount.c

diagfs: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o diagfs

clean:
	rm -f diagfs
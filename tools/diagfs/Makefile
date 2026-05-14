CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -I../../libdiag/include
# 把 LDFLAGS 改成 LIBS，讓下面的指令抓得到
LIBS    = ../../libdiag/libdiag.a

# 將所有拆分出來的來源檔案列出來
SRCS = diagfs.c diagfs_analyze.c diagfs_policy.c diagfs_output.c diagfs_mount.c

diagfs: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o diagfs

clean:
	# 把 $(TARGET) 直接改成 diagfs
	rm -f diagfs
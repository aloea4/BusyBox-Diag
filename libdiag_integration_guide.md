# libdiag Integration Guide


## 0. API Function Mapping

本節說明上層工具呼叫哪些 libdiag 函式，以及會取得哪些資料。

| 上層工具 | 使用函式 | 取得資料 | 用途 |
|---|---|---|---|
| diagps | `diag_proc_foreach()` | 多個行程的 PID、PPID、state、RSS、utime、stime、comm | 列出行程清單 |
| diagps | `diag_proc_read(pid, &info)` | 單一 PID 的行程資訊 | 查詢指定行程 |
| diagps | `diag_cpu_read_snapshot()` | CPU 累積 tick snapshot | 計算 CPU 使用率 |
| diagps | `diag_cpu_total()` | CPU total ticks | 差分計算分母 |
| diagps | `diag_cpu_idle()` | idle + iowait ticks | 計算 idle ratio |
| diagfs | `diag_fs_read(path, &info)` | 容量、free、used、inode、fs_type | 類似 df / inode 檢查 |
| diagfs | `diag_fs_type_name(fs_type)` | filesystem 類型名稱 | 顯示 proc/tmpfs/ext 等 |
| diagfs | `diag_fs_read_fiemap(path, &info)` | extent_count、logical_bytes、physical_bytes | 檔案碎片化估算 |
| diagnet | `diag_net_foreach_tcp()` | TCP local/remote IP、port、state | 列出 TCP 連線 |
| diagnet | `diag_net_foreach_udp()` | UDP local/remote IP、port、state | 列出 UDP 連線 |
| common | `diag_strerror(err)` | 錯誤碼文字 | 統一錯誤訊息 |

## 1. 編譯與連結

使用 libdiag 時，需要：

```bash
gcc -Wall -Wextra -std=c99 -Iinclude your_tool.c libdiag.a -o your_tool
````

重點：

```text
-Iinclude   讓 compiler 找到 include/libdiag/*.h
libdiag.a   靜態連結 libdiag
```

---

## 2. Process / CPU 模組

### Include

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"
```

### 範例：列出行程

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"

#include <stdio.h>

static int print_proc(const diag_proc_info_t *p, void *user)
{
    (void)user;

    printf("%d %d %c %lu %s\n",
           p->pid,
           p->ppid,
           p->state,
           p->rss_kb,
           p->comm);

    return 0;
}

int main(void)
{
    int ret;

    ret = diag_proc_foreach(print_proc, NULL);
    if (ret != DIAG_OK) {
        printf("diag_proc_foreach failed: %s\n", diag_strerror(ret));
        return 1;
    }

    return 0;
}
```

### 範例：讀 CPU snapshot

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"

#include <stdio.h>

int main(void)
{
    diag_cpu_snapshot_t cpu;
    int ret;

    ret = diag_cpu_read_snapshot(&cpu);
    if (ret != DIAG_OK) {
        printf("cpu read failed: %s\n", diag_strerror(ret));
        return 1;
    }

    printf("total=%llu idle=%llu\n",
           diag_cpu_total(&cpu),
           diag_cpu_idle(&cpu));

    return 0;
}
```

---

## 3. Filesystem 模組

### Include

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"
```

### 範例：讀取 filesystem 使用量

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>

int main(void)
{
    diag_fs_info_t fs;
    int ret;

    ret = diag_fs_read("/", &fs);
    if (ret != DIAG_OK) {
        printf("diag_fs_read failed: %s\n", diag_strerror(ret));
        return 1;
    }

    printf("path=%s type=%s total=%luKB used=%luKB free=%luKB\n",
           fs.path,
           diag_fs_type_name(fs.fs_type),
           fs.total_kb,
           fs.used_kb,
           fs.free_kb);

    return 0;
}
```

### 範例：FIEMAP extent 數量

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>

int main(void)
{
    diag_fiemap_info_t info;
    int ret;

    ret = diag_fs_read_fiemap("Makefile", &info);
    if (ret == DIAG_ERR_UNSUPPORTED) {
        printf("FIEMAP unsupported\n");
        return 0;
    }

    if (ret != DIAG_OK) {
        printf("FIEMAP failed: %s\n", diag_strerror(ret));
        return 1;
    }

    printf("extent_count=%u logical_bytes=%llu\n",
           info.extent_count,
           info.logical_bytes);

    return 0;
}
```

---

## 4. Network 模組

### Include

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_net.h"
```

### 範例：列出 TCP 連線

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_net.h"

#include <stdio.h>

static int print_conn(const diag_net_conn_t *c, void *user)
{
    (void)user;

    printf("%s %s:%u -> %s:%u %s\n",
           c->proto,
           c->local_addr,
           c->local_port,
           c->remote_addr,
           c->remote_port,
           c->state);

    return 0;
}

int main(void)
{
    int ret;

    ret = diag_net_foreach_tcp(print_conn, NULL);
    if (ret != DIAG_OK) {
        printf("diag_net_foreach_tcp failed: %s\n", diag_strerror(ret));
        return 1;
    }

    return 0;
}
```

---

## 5. Callback 規則

`diag_proc_foreach()`、`diag_net_foreach_tcp()`、`diag_net_foreach_udp()` 都使用 callback。

```text
callback return 0     → 繼續
callback return 非 0  → 停止
```

範例：只印前 10 筆：

```c
static int print_first_10(const diag_proc_info_t *p, void *user)
{
    int *count = (int *)user;

    printf("%d %s\n", p->pid, p->comm);

    (*count)++;
    if (*count >= 10) {
        return 1;
    }

    return 0;
}
```

呼叫：

```c
int count = 0;
diag_proc_foreach(print_first_10, &count);
```

---

## 6. 錯誤處理規則

所有 API 回傳：

```c
DIAG_OK
DIAG_ERR_IO
DIAG_ERR_PARSE
DIAG_ERR_INVALID_ARG
DIAG_ERR_UNSUPPORTED
DIAG_ERR_BUFFER_TOO_SMALL
```

取得錯誤字串：

```c
printf("%s\n", diag_strerror(ret));
```

---

## 7. 給三個工具的建議接法

### diagps

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"
```

使用：

```c
diag_proc_foreach(...)
diag_cpu_read_snapshot(...)
```

---

### diagfs

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"
```

使用：

```c
diag_fs_read(...)
diag_fs_read_fiemap(...)
```

---

### diagnet

```c
#include "libdiag/diag_common.h"
#include "libdiag/diag_net.h"
```

使用：

```c
diag_net_foreach_tcp(...)
diag_net_foreach_udp(...)
```

---

## 8. 注意事項

```text
1. 上層工具不要直接解析 /proc
2. 上層工具不要直接呼叫 statfs 或 FIEMAP
3. 上層工具只負責 CLI、輸出格式、使用者互動
4. libdiag 只負責資料收集與 struct 封裝
```


## 9. Project Structure & Integration Layout

本專案採用「分層設計（Layered Architecture）」：

```text
final_project/
│
├── libdiag/
│   ├── include/libdiag/
│   ├── src/libdiag/
│   └── libdiag.a
│
├── tools/
│   ├── diagps/
│   ├── diagfs/
│   ├── diagnet/
│
├── examples/
├── tests/fixtures/
├── docs/
```

---

## 📌 Layered Design

```
Layer 1：libdiag（資料收集）
Layer 2：tools（CLI / 顯示）
```

---

## 📌 Data Flow

```
/proc / statfs / ioctl
        ↓
     libdiag
        ↓
diagps / diagfs / diagnet
```
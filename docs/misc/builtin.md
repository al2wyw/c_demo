## `__` 开头符号的三种归属规律

C 标准规定：`__` 开头的标识符是"实现保留"的。实际中它们分三类：

| 归属 | 典型例子 | 定义位置 |
|------|----------|----------|
| **① 编译器 built-in** | `__atomic_fetch_add`、`__builtin_expect`、`__sync_bool_compare_and_swap`、`__thread` | 编译器内部，**无头文件** |
| **② 编译器预定义宏** | `__ATOMIC_SEQ_CST`、`__GNUC__`、`__x86_64__`、`__FILE__`、`__LINE__` | 编译器内部，**无头文件** |
| **③ 标准库私有实现** | `__pthread_mutex_lock`、`__errno_location`、`__libc_start_main` | glibc / libc 头文件 + `.so` |

**如何区分？**
- 若 `grep -rn` 在 `/usr/include/` 找不到 → 大概率是 ① 或 ②
- 若 `clang -dM -E` 能看到 → 是 ②
- 若 `nm` 显示为未定义符号 `U` → 是 ③
- 若 `nm` 里没有它但代码能编译过 → 是 ①（被编译器就地展开）
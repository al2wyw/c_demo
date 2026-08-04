### `__user` 的核心含义

`__user` 是一个用于**区分用户空间指针**的类型标注宏，主要供 **Sparse 静态检查工具**使用，在编译期发现"内核代码错误地直接解引用用户空间指针"的 bug。

### 为什么需要它？

Linux 内核运行在特权态，可以访问任何地址。但用户传入的指针可能：
1. **无效**（指向未映射的地址，导致内核 panic）
2. **恶意构造**（指向内核内存，导致越权读写 → 安全漏洞）

所以通过 `__user` 标记 + Sparse 检查，可以在**编译期静态发现**违规的直接解引用

例如 `struct sigcontext __user *sc` 表示 `sc` 是一个**指向用户空间的 sigcontext 结构体指针**。它告诉阅读者和 Sparse：
- 这块内存位于**用户态**（进程的用户栈上）
- **不能在内核中直接解引用** `sc->xxx`（会造成安全漏洞、页错误等问题）
- 必须通过安全的访问函数来读写，例如：
    - `copy_from_user()` / `copy_to_user()`
    - `get_user()` / `put_user()`
    - `__get_user()` / `__put_user()`（带 `access_ok()` 检查后使用）

### 相关的宏

在 [compiler_types.h](/linux/include/linux/compiler_types.h) 中还定义了一组地址空间标注：

| 宏 | 含义 |
|----|------|
| `__kernel` | 普通的内核空间指针（默认，通常省略） |
| `__user` | 用户空间指针 |
| `__iomem` | I/O 内存映射空间（MMIO） |
| `__percpu` | Per-CPU 变量 |
| `__rcu` | 受 RCU 保护的指针 |
| `__force` | 强制转换（用于跨地址空间转换时抑制 Sparse 警告） |

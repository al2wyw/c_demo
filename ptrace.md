在用户态，你**无法直接读到**"某个正在被内核调度出去、躺在运行队列里"的线程的 CPU 寄存器（PC/SP/FP…）。原因很简单：那些寄存器值被内核保存在**内核栈**里（`task_struct->thread.sp/ip`、`pt_regs` 等），用户态没有合法路径去读。

所以要"抓到另一个线程的上下文并改 PC"，实际做法只有两条路：

| 路径 | 机制 | 谁来拿到 `ucontext_t` |
|---|---|---|
| **A. 信号注入**（同进程内，用户态） | `pthread_kill` / `tgkill` 给目标线程发信号，内核在**目标线程**返回用户态时投递信号，构造 `ucontext_t` 交给 handler | 目标线程自己在 signal handler 里拿到 |
| **B. ptrace / 调试接口**（跨进程，或外部工具） | 让内核把线程"停下"，通过 `PTRACE_GETREGS` / `PTRACE_SETREGS` 直接读写 `pt_regs` | 调试者（外部进程）拿到 |

第三种"看起来像"的 `getcontext()/setcontext()` 只能对**当前线程自己**用，取不到别的线程的上下文
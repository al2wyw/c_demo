## 1. 先纠正上次的错误说法

上次我说"thread-local polling page 让每个线程一行 cache line，避免全局 cache line 争用" —— 这个说法**在两个层面上不严谨甚至有误**：

### 错误一：弄反了 polling page 的本质
看 [safepointMechanism.cpp](/Users/liyang/workspace/jdk15/src/hotspot/share/runtime/safepointMechanism.cpp) 第 47–62 行：

```cpp
const size_t allocation_size = 2 * page_size;
char* polling_page = os::reserve_memory(allocation_size, NULL, page_size);
...
char* bad_page  = polling_page;
char* good_page = polling_page + page_size;

os::protect_memory(bad_page,  page_size, os::MEM_PROT_NONE);
os::protect_memory(good_page, page_size, os::MEM_PROT_READ);
...
poll_armed_value    |= bad_page_val;   // 指向不可读页
poll_disarmed_value |= good_page_val;  // 指向可读页
```

**全 JVM 只有一对 polling page（bad/good 各一页）**，而且这对页的保护位**几乎从不修改**——它们是常量地址，分别永久代表 "armed" 和 "disarmed"。这是 JEP 312 的关键改造之一：**不再像旧实现那样依赖 `mprotect()` 切换全局页保护**（旧方案的 `mprotect` 是真正的瓶颈：要发 IPI 让所有 CPU 失效 TLB）。

---

## 2. 直面你的核心质疑

> "STW 时本来就是要把所有线程的 cache line 都写一遍，跟全局用一个 cache line 有什么区别？"

**你的质疑成立，而且非常关键**：从"修改总量"上看，per-thread 方案 STW 时确实要写 N 个线程的 cache line，**总写入次数比全局方案多**。所以单纯说"消除了 cache line 争用"是错的。但二者实际有以下几个本质差异，per-thread 方案的优势不在"总写入量"，而在以下几点：

### (A) 旧全局方案的真正瓶颈不是"一行 cache line"，而是 `mprotect` + IPI

旧的全局 polling page 方案，arm 的方式是 `mprotect(global_page, PROT_NONE)`：

- `mprotect` 要修改进程页表，必须让所有 CPU 的 **TLB shootdown**；
- 内核会向所有相关 CPU 发 **IPI（处理器间中断）**，每个 CPU 都得停下来 invalidate TLB；
- 这条路径在大核机器上是**几十微秒级**的延迟，且每次 STW arm 一次、disarm 一次，**每个 safepoint 至少两次 mprotect + 两次 IPI 风暴**。

per-thread 方案完全**不再调用 mprotect** —— 那两个 polling page 的保护位是 JVM 启动时一次性设好，**终身不变**。VM 要 arm 的时候只是改各线程对象里的一个指针字段，**这是普通内存写，没有任何 syscall、没有任何 IPI**。

> 所以 per-thread 方案省下来的不是"一行 cache line"，而是**整套内核 mprotect/TLB shootdown 路径**——这才是它最大的性能收益。

### (B) "写 N 行" vs "写 1 行 + 让 N 个核读" 不是同量级的事

| 维度 | 全局 polling page | per-thread polling |
|---|---|---|
| arm 时 VMThread 写多少次 | 1 次 mprotect | N 次普通 store |
| 是否需要 syscall | 是 | 否 |
| 是否触发 TLB shootdown / IPI | 是（向所有 CPU 发 IPI） | 否 |
| 业务线程感知到 arm 的方式 | TLB invalidate → page fault | 普通的 cache coherence（MESI） |
| 对 VMThread 的开销 | O(CPU 数) 的内核同步 | O(线程数) 的 store |
| 业务线程在"无事 polling"时 | 反复读同一全局只读页（共享 S 状态，无争用） | 反复读自己 `_polling_page` 字段 |

注意第 4 行的对比：per-thread 方案的"通知"靠普通的 **MESI 协议**完成，VMThread 的 store 会让对应业务线程所在 CPU 的那条 cache line 转 Invalid，下一次该业务线程 poll 时会从 VMThread 的 cache 拉过来——这是硬件本来就在做的事情，**不需要 OS 介入**。

而旧方案虽然单点写少，但通知方式是 mprotect + IPI，**每次都要把所有 CPU 拽进内核态做 TLB invalidate**，这个跨 CPU 内核同步的开销远比 N 次普通 cache 同步要大。

### (C) per-thread 真正的"无争用"指的是 polling 时的读端，而不是 arm 时的写端

热路径上业务线程做的是：
```asm
mov  rscratch1, [r15 + Thread::polling_page_offset]  ; 读自己的 _polling_page 字段
test eax, [rscratch1]                                 ; 读 bad_page 或 good_page
```

- `_polling_page` 字段在 `JavaThread` 对象里，几乎只被该线程自己反复读取；
- 写它的只有 VMThread（arm/disarm 时一次），或者线程自己（disarm_local_poll_release）；
- 平稳运行时，业务线程缓存的就是这条 line 的 **E（独占）或 S（共享）状态**，不会有跨核 ping-pong；
- 第二步 `test [bad/good_page]` 也是只读 + 全局共享只读页，所有 CPU 都以 S 状态命中，**没有任何 cache line 争用**。

如果是旧方案，**热路径**也只是读一条全局只读 line（同样无争用）——所以这一点上两者**热路径几乎打平**，真正的差异在 STW 路径（见 A）。

### (D) per-thread 方案带来的**功能性**好处（这是 JEP 312 的初衷）

更重要的是，per-thread 字段使得 **handshake**（针对单一线程的轻量 safepoint）成为可能：
- VMThread 可以只 arm 某一个线程的 `_polling_page`，让它单独进入 safepoint 处理一次（如 deopt、stack walk、JVMTI 回调），而**其他线程完全不感知、继续跑**；
- 这是全局 polling page 方案无法做到的——arm 全局页就是全局 STW。

JEP 312（Thread-Local Handshakes）的标题就是它，性能优势分析里更核心的卖点其实是这个，而不是"省 cache line 争用"。

---

## 3. 一句话修正后的结论

> per-thread polling 的优势**不在于减少 cache line 争用**（你指出得对，STW 时反而要写 N 行 cache line），而在于：
> - 把"通知所有线程进入 safepoint"这件事，从 **`mprotect` + 内核 TLB shootdown + IPI 风暴** 降级为 **N 次普通用户态 store + 硬件 MESI**；
> - 同时解锁了 **handshake**（单线程 safepoint）这个 STW 全局方案做不到的能力。

感谢指正，上次那段"避免全局 cache line 争用"的措辞不准确，正确的说法应该是"**避免了全局 mprotect/IPI 路径**"。
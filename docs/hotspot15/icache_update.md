mfence 不会去动本核的 L1I，在x86 规范里 IFU(指令获取器) 不参与常规的 D-cache 一致性协议，所以写进 L1D 的新指令不会更新到L1I，既本核的 L1I 里可能还缓存着旧指令。

在自修改代码（Self-Modifying Code, SMC）和跨核代码 patch（Cross-Modifying Code, XMC）的场景下(HotSpot中的IC patch、NativeCall patch、C1/C2 代码生成、deoptimize等)需要 clflush 指令去把包含 addr 的那条 cache line 从整个 cache 层次结构（所有核的 L1D、L2、L3，L1I）中失效掉。

所以 x86 上刷 I-cache 的正确模板就是：mfence + clflush × N + mfence，一个都不能少。
- 没有第一个 mfence：clflush 可能刷到 patch 前的旧数据。
- 没有 clflush：L1I 里的旧指令永远不会被丢弃，mfence 再多次也没用。
- 没有第二个 mfence：clflush 的效果可能还没生效，后续的 call 就已经把旧指令取进流水线了。
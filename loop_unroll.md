```text
[johnnyleeli@VM-148-55-centos ~]$ gcc loop_unroll.c time_utils.c -O3 -fno-tree-vectorize -o loop_o3
[johnnyleeli@VM-148-55-centos ~]$ ./loop_o3 100000000
ret:1860272594, time: 83051000
ret:1860272594, time: 75990000
ret:1860272594, time: 74936000
ret:1860272594, time: 73567000
ret:1860272594, time: 73545000
[johnnyleeli@VM-148-55-centos ~]$ lscpu
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                16
On-line CPU(s) list:   0-15
Thread(s) per core:    1
Core(s) per socket:    16
Socket(s):             1
NUMA node(s):          1
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 85
Model name:            Intel(R) Xeon(R) Platinum 8255C CPU @ 2.50GHz
Stepping:              5
CPU MHz:               2494.142
BogoMIPS:              4988.28
Hypervisor vendor:     KVM
Virtualization type:   full
L1d cache:             32K
L1i cache:             32K
L2 cache:              4096K
L3 cache:              36608K
NUMA node0 CPU(s):     0-15
Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl nonstop_tsc eagerfpu pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch fsgsbase bmi1 hle avx2 smep bmi2 erms invpcid rtm mpx avx512f avx512dq rdseed adx smap clflushopt clwb avx512cd avx512bw avx512vl xsaveopt xsavec xgetbv1 arat
[johnnyleeli@VM-148-55-centos ~]$ objdump -S loop_o3
loop_o3:     file format elf64-x86-64

00000000004008a0 <base>:
  4008c8:       49 03 1c d4             add    (%r12,%rdx,8),%rbx
  4008cc:       48 83 c2 01             add    $0x1,%rdx
  4008d0:       39 d5                   cmp    %edx,%ebp
  4008d2:       7f f4                   jg     4008c8 <base+0x28>

0000000000400910 <test0>:
  400940:       48 03 1f                add    (%rdi),%rbx
  400943:       48 83 c7 20             add    $0x20,%rdi
  400947:       48 03 5f e8             add    -0x18(%rdi),%rbx
  40094b:       48 03 5f f0             add    -0x10(%rdi),%rbx
  40094f:       48 03 5f f8             add    -0x8(%rdi),%rbx
  400953:       48 39 d7                cmp    %rdx,%rdi
  400956:       75 e8                   jne    400940 <test0+0x30>

0000000000400990 <test1>:
  4009c8:       4c 03 07                add    (%rdi),%r8
  4009cb:       48 03 77 08             add    0x8(%rdi),%rsi
  4009cf:       48 83 c7 20             add    $0x20,%rdi
  4009d3:       48 03 4f f0             add    -0x10(%rdi),%rcx
  4009d7:       48 03 57 f8             add    -0x8(%rdi),%rdx
  4009db:       48 39 c7                cmp    %rax,%rdi
  4009de:       75 e8                   jne    4009c8 <test1+0x38>

0000000000400a20 <test2>:
  400a50:       48 03 1f                add    (%rdi),%rbx
  400a53:       48 83 c7 20             add    $0x20,%rdi
  400a57:       48 03 5f e8             add    -0x18(%rdi),%rbx
  400a5b:       48 03 5f f0             add    -0x10(%rdi),%rbx
  400a5f:       48 03 5f f8             add    -0x8(%rdi),%rbx
  400a63:       48 39 d7                cmp    %rdx,%rdi
  400a66:       75 e8                   jne    400a50 <test2+0x30>

0000000000400aa0 <test3>:
  400ad0:       48 8b 17                mov    (%rdi),%rdx
  400ad3:       48 03 57 08             add    0x8(%rdi),%rdx
  400ad7:       48 83 c7 20             add    $0x20,%rdi
  400adb:       48 03 57 f0             add    -0x10(%rdi),%rdx
  400adf:       48 03 57 f8             add    -0x8(%rdi),%rdx
  400ae3:       48 01 d3                add    %rdx,%rbx
  400ae6:       48 39 cf                cmp    %rcx,%rdi
  400ae9:       75 e5                   jne    400ad0 <test3+0x30>
```

# 深入分析：test0 vs test1 的性能差异

## 关键路径再分析

### test0 的循环体（400940~0x400956）

```asm
400940: add    (%rdi),%rbx          ; rbx = rbx + arr[i]
400943: add    $0x20,%rdi           ; rdi += 32 (指针递增)
400947: add    -0x18(%rdi),%rbx     ; rbx = rbx + arr[i+1]
40094b: add    -0x10(%rdi),%rbx     ; rbx = rbx + arr[i+2]
40094f: add    -0x8(%rdi),%rbx      ; rbx = rbx + arr[i+3]
400953: cmp    %rdx,%rdi
400956: jne    400940
```

**关键路径在 `%rbx`**：4 条 add 指令串行依赖 rbx，形成一条长度为 4 的依赖链。
- Skylake (8255C 是 Cascade Lake/Skylake-SP) 上 `add reg, [mem]` 的延迟是 **memory load (4-5 cycle) + add (1 cycle)**，但 load 和 add 可以流水化。
- **关键依赖链上每个 add 的延迟是 1 cycle**（load 部分不在 rbx 的依赖链上，因为 load 不依赖 rbx）。
- 所以每次循环（4 个元素）关键路径理论上 **4 cycle**，即 **1 cycle/元素**。

### test1 的循环体（4009c8~4009de）

```asm
4009c8: add    (%rdi),%r8           ; r8 += arr[i]
4009cb: add    0x8(%rdi),%rsi       ; rsi += arr[i+1]
4009cf: add    $0x20,%rdi
4009d3: add    -0x10(%rdi),%rcx     ; rcx += arr[i+2]
4009d7: add    -0x8(%rdi),%rdx      ; rdx += arr[i+3]
4009db: cmp    %rax,%rdi
4009de: jne    4009c8
```

**关键路径**：4 个独立的累加器 r8、rsi、rcx、rdx，每个累加器自己的依赖链长度为 1（每次循环只 add 一次）。
- 每次循环关键路径理论上 **1 cycle**，即 **0.25 cycle/元素**。

## 你的疑问：理论上 test1 应该快 4 倍，为什么没区别？

**答案确实是瓶颈转移到了执行资源上**，但不是"内存读写端口不够"那么简单，让我们算清楚。

### Skylake-SP 的执行端口约束

Intel Xeon Platinum 8255C (Skylake-SP) 每个核心：

| 资源 | 数量/cycle |
|---|---|
| Load 端口 (port 2, 3) | **2 loads/cycle** |
| ALU (port 0,1,5,6 都能做简单 add) | 4 |
| 融合域指令吞吐 | 4 uops/cycle (前端) |
| 分支端口 | 1 (port 0 或 6) |

### 实际瓶颈分析

`add reg, [mem]` 是一个融合宏指令，但在执行时拆成 2 个 uop：1 个 load + 1 个 add。

**test0 每次循环（4 个元素）**：
- 4 个 load uops
- 4 个 add uops  
- 1 个 `add $0x20, %rdi`
- 1 个 cmp + jne (融合为 1 个 uop)
- 总 uops：约 10，其中 **4 loads**

按 load 端口约束：**4 loads / 2 loads-per-cycle = 2 cycle/iter = 0.5 cycle/元素**

**test1 每次循环**：uops 数量几乎一样，**load 数量也是 4 个**。

### 实测数据验证

```
ret:1860272594, time: 75990000   // test0
ret:1860272594, time: 74936000   // test1
```

`100,000,000` 元素，0.075 秒 → **每元素 ~0.75 ns**

CPU 2.5 GHz → 每 cycle 0.4 ns → **每元素 ~1.87 cycle**

这个数字比"load 端口瓶颈 0.5 cycle"要慢，原因可能是：
1. **L1 cache 未命中**：数组 `100,000,000 * 8 bytes = 800MB`，远超 L3 (36MB)，**这其实是 DRAM 带宽瓶颈**。
2. 内存带宽约 800MB / 0.075s ≈ **10.7 GB/s**，对于单核访问 DRAM 来说是合理的。

## 真正的瓶颈

```
🔴 瓶颈不是 CPU 关键路径，而是【DRAM 内存带宽】
```

让我们对比 base（无展开）：
```
ret:1860272594, time: 83051000   // base, 关键路径 1 cycle/元素
ret:1860272594, time: 75990000   // test0
ret:1860272594, time: 74936000   // test1
```

base 的 CPU 关键路径理论上是 1 cycle/元素 = 0.4 ns/元素，而实际是 0.83 ns/元素 → **base 已经接近内存带宽瓶颈**，但还残留一点 CPU 关键路径开销。

test0/test1 进一步消除了 CPU 侧的次要开销，**完全压在内存带宽上**，因此两者几乎相同。

## 验证建议

如果你想看到 test0 和 test1 的 4× 差异，把 `LOOP` 改小到能放进 L1 cache（比如 2048 个 long = 16KB），并多次循环累加：

```c
const int LOOP = 2048;       // 数组大小放进 L1
const int REPEAT = 50000;    // 外层循环

for (int r = 0; r < REPEAT; r++) {
    for (int i = 0; i < LOOP; i+=4) { ... }
}
```

这样去掉内存瓶颈，test1 应该比 test0 快接近 4 倍（受限于 2 loads/cycle，实际约 2-3 倍）。
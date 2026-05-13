[任务规划完成]
结合 [asm.txt](/Users/liyang/workspace/jdk15/asm.txt) 的实际汇编来精确分析。

## 重新审视关键路径

先把主循环的完整指令序列和依赖关系画清楚：

```asm
; ── 循环入口 ──
0x2b31: mov    %r10d,%edx        ; edx = operand1（无依赖，从寄存器读）
0x2b34: mov    %r9d,%ecx         ; ecx = i
0x2b37: add    %r10d,%ecx        ; ecx = i + operand1

; ── 16 次 add 串行链 ──
0x2b3a ~ 0x2b58: add %ecx,%edx  ; ×16，RAW 依赖链

; ── 写回 operand2（第一次）──
0x2b5a: mov    %r9d,%r11d
0x2b5d: add    $0xe,%r11d        ; r11d = i + 14
0x2b61: mov    %r11d,0x14(%rbx)  ; store operand2

; ── 修正 + 写回 ret ──
0x2b65: add    $0x78,%edx        ; edx += 120
0x2b68: mov    %edx,0xc(%rbx)    ; store ret  ← 依赖 16次add链的末尾

; ── 写回 operand2（第二次）──
0x2b6b: mov    %r9d,%ecx
0x2b6e: add    $0xf,%ecx         ; ecx = i + 15
0x2b71: mov    %ecx,0x14(%rbx)   ; store operand2

; ── 循环控制 ──
0x2b74: add    $0x10,%r9d        ; i += 16
0x2b78: cmp    %r8d,%r9d
0x2b7b: jl     0x2b34            ; 回跳
```

---

## 精确的关键路径（携带循环间依赖）

注意循环回跳后，下一轮的 `mov %r10d,%edx` 会**覆盖 edx**，所以 `edx` 的循环间依赖链**在每轮入口被打断**。

真正的循环间依赖是 **`r9d`（i 变量）**：

```
本轮：add $0x10,%r9d   →   下轮：mov %r9d,%ecx → add %r10d,%ecx → 16×add
```

### 本轮关键路径（从入口到回跳）

```
mov %r9d,%ecx          : 0 cycles（寄存器 rename，消除）
add %r10d,%ecx         : 1 cycle
16 × add %ecx,%edx     : 16 cycles（串行 RAW 链）
add $0x78,%edx         : 1 cycle
mov %edx,0xc(%rbx)     : 4 cycles（L1 store，但不在关键路径！）
add $0x10,%r9d         : 1 cycle（与上面并行）
cmp + jl               : 1 cycle
─────────────────────────────────────
关键路径：1 + 16 + 1 + 1 + 1 = 20 cycles
```

> `mov %edx,0xc(%rbx)` 这条 store 虽然依赖 `edx`，但它**不是下一轮迭代的输入**，乱序执行引擎可以在它提交之前就开始下一轮——所以它**不在循环间关键路径上**。

---

## 如果用位移替代 16 次 add，关键路径变为多少？

### 替换方案

```asm
; 原来：16 × add %ecx,%edx（16 cycles）
; 替换为：
mov    %ecx,%r11d        ; 0 cycles（rename）
shl    $4,%r11d          ; 1 cycle（r11d = ecx × 16）
add    %r11d,%edx        ; 1 cycle（edx += r11d）
add    $0x78,%edx        ; 1 cycle（修正 +120，原本就有）
```

### 替换后关键路径

```
mov %r9d,%ecx            : 0 cycles
add %r10d,%ecx           : 1 cycle
shl $4,%r11d             : 1 cycle（与 ecx 计算串行）
add %r11d,%edx           : 1 cycle
add $0x78,%edx           : 1 cycle
add $0x10,%r9d           : 1 cycle（并行）
cmp + jl                 : 1 cycle
─────────────────────────────────────
关键路径：1 + 1 + 1 + 1 + 1 = 5 cycles
```

---

## 对比总结

| 指标 | 原始（16×add） | 位移优化后 |
|------|--------------|-----------|
| 关键路径延迟 | **20 cycles** / 轮 | **5 cycles** / 轮 |
| 指令数（主循环体） | ~25 条 | ~12 条 |
| 循环间依赖链 | r9d：1 cycle | r9d：1 cycle（不变） |
| 吞吐量提升 | 基准 | **理论 4×** |

---

## 结论：位移优化是合理且有效的

之前说"瓶颈在内存写回"是错误的表述。**正确结论是**：

1. **`mov %edx,0xc(%rbx)` 不在循环间关键路径上**——它的结果不被下一轮迭代消费，乱序执行可以将其与下一轮的计算重叠执行

2. **真正的瓶颈就是 16 次 add 的串行依赖链（16 cycles）**，占关键路径的 80%

3. **用 `shl $4 + add` 替代 16 次 add 是合理优化**，可将每轮关键路径从 20 cycles 压缩到 5 cycles，理论吞吐量提升约 **4 倍**

4. C2 没有做这个优化的原因是：`clone_loop` 只做节点复制，展开后不触发跨副本的代数化简（`16×AddI → ShlI + AddI`），这是 C2 优化 pipeline 的设计局限，而非"不值得优化"
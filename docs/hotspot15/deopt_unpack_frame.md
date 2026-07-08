

# 编译栈退优化为解释栈的完整过程

我先简单说结论：**新解释帧不是"加塞到 stub 帧和 caller 帧之间"，而是把被 deopt 的整个编译帧（连同 stub 帧）弹掉，然后重新 push 出多个骨架解释帧、最后再 push 一个 stub 帧回来重跑一次**。整个过程分两阶段：**先在 Java 侧写好数据（vframeArray），再回到汇编 stub 里物理地重建栈**。

## 一、场景假设

假设编译方法 `outer()` inline 了 `inner()`。运行到 `inner` 中的某 bci_i 时触发 deopt。需要展开成 2 个解释帧（因为 inline 层数=2）：

- `caller()`：调用 outer 的方法（不动，是"caller frame"）。
- `outer()`：本来是被 inline 掉的、编译版本里没有独立栈帧，deopt 后要还原成一个解释帧。**这是较老/底层的帧**（index = frames()-1 = 1）。
- `inner()`：也是被 inline 的、要还原成一个解释帧。**这是最年轻/顶层的帧**（index = 0）。

## 二、Stage 0：deopt 触发那一刻的栈

```
高地址（外层，older）
├───────────────────────────────────────┤
│   caller() 的栈帧（不变）             │
│   ...                                 │
├───────────────────────────────────────┤ ← caller sp
│   outer() 编译帧 ★"被 deopt 的帧"     │
│   （里面 inline 了 inner，物理上只有  │
│    这一个编译帧，双重逻辑活动都在这）  │
│                                       │
│   locals / spills ...                 │
│   orig_pc slot = PC_B  ← 已备份好     │
│   saved rbp                           │
│   return addr → caller 里 call outer  │
│                 之后的下条 = PC_caller│
├───────────────────────────────────────┤ ← deoptee sp
│   safepoint / call stub 帧            │
│   保存的所有 caller-saved regs        │
│   return addr = deopt_handler_entry   │ ← 已经被 patch_pc 改过了
├───────────────────────────────────────┤ ← 栈顶
低地址
```

调用链：`caller → outer(编译, inline了 inner) → stub`。

## 三、Stage 1：进入 deopt_blob → fetch_unroll_info（还没动栈）

`baz ret` 之后跳到 `deopt_handler_entry`，随即由 deopt_blob 汇编代码调用 `Deoptimization::fetch_unroll_info` 进入 C++。这时又生成了一个"deopt stub"的栈帧压在栈顶。栈变成：

```
├───────────────────────────────────────┤
│   caller() 帧          （不变）       │
├───────────────────────────────────────┤
│   outer() 编译帧       （不变）       │
├───────────────────────────────────────┤
│   deopt stub 栈帧（deopt_blob 的）    │
│   保存返回值寄存器、传参给 C++        │
├───────────────────────────────────────┤ ← ★"unpack_frame / stub_frame"
```

在这个新 stub 帧里，fetch_unroll_info_helper 干下面几件事（**只在 C heap 里搭建元数据，不动栈上任何东西**）：

1. `stub_frame.sender()` 找到 deoptee = `outer` 编译帧。
2. `vframe::new_vframe(deoptee)` 把 inline 层次展开成 `chunk` 数组：`chunk[0] = inner (最年轻)`, `chunk[1] = outer (最老)`。共 2 个 compiledVFrame。
3. `create_vframeArray(...)`：
    - `new vframeArrayElement[2]` 保存两个解释帧的"数据快照"。
    - 每个 element 里 `fill_in()`：把该层的 method、bci、locals、expression stack、monitors 全部**拷贝到 C heap**（不再依赖编译帧的物理布局，因为编译帧马上要被覆盖）。
4. 计算 `frame_sizes[]`：每个解释骨架帧需要多大栈空间（用 `Interpreter::size_activation` 算）。
5. 计算 `frame_pcs[]`：每个骨架帧的 return address（第 0 号是 caller 的 pc，其余填 `Interpreter::deopt_entry(vtos, 0)` 作占位）。
6. 计算 `caller_adjustment`：如果 caller 是编译帧或 method handle 调用，最老那个解释帧可能没法直接坐在 caller 上，需要在 caller 上多扩几个字给 outer 的 locals 用。
7. 打包成 `UnrollBlock` 返回给 deopt_blob 汇编代码。

注意此时 `outer` 编译帧的物理内存**还完好地压在栈上**，没被覆盖。

## 四、Stage 2：deopt_blob 汇编回来后——**物理弹栈**

fetch_unroll_info 返回后，deopt_blob 汇编根据 UnrollBlock 做下面的动作（x86 上是 `SharedRuntime::generate_deopt_blob` 生成的代码）：

**第一步：弹掉 stub 帧 + outer 编译帧**

用 `size_of_deoptimized_frame`（outer 编译帧的大小）把 rsp 往回抬，跳过 outer 帧。同时 stub 帧也被跳过。相当于 `pop` 掉最上面两层。

```
├───────────────────────────────────────┤
│   caller() 帧                         │
├───────────────────────────────────────┤ ← rsp 现在指到这（caller 的 sp 位置）
        (outer 编译帧的物理内存变成"栈以下"，逻辑上已经释放)
```

**第二步：扩 caller（可选）**

如果 `caller_adjustment > 0`，把 rsp 再向下（低地址）压 caller_adjustment 个字节。这些字**逻辑上归属于 caller 帧的顶部**，是给"最老的解释帧"（outer 解释版）当 locals 用的。

## 五、Stage 3：push 出 2 个骨架解释帧

deopt_blob 有一个循环，按照 `frame_sizes[]` 从**老到新**依次 push 骨架帧（每个骨架帧内容就是"预留出足够空间 + 一个 return address"）：

```
├───────────────────────────────────────┤
│   caller() 帧                         │
│   (若 caller_adjustment>0：多出的     │
│    outer locals 区)                   │
├───────────────────────────────────────┤ ← outer 解释帧的顶
│   outer() 骨架解释帧  (frame_sizes[0])│
│   locals / expression / monitor ...   │
│   (内容是空壳，还没填)                │
│   saved rbp                           │
│   return addr = frame_pcs[1]          │  ← 老到新 push，先 push 这个
│                = Interpreter::deopt_entry(vtos,0) - offset  (占位)
├───────────────────────────────────────┤ ← outer 解释帧的底 = inner 解释帧的顶
│   inner() 骨架解释帧  (frame_sizes[1])│
│   locals / expression / monitor ...   │
│   saved rbp                           │
│   return addr = frame_pcs[2]          │  ← 最后 push 这个
│                = Interpreter::deopt_entry(vtos,0) (占位)
├───────────────────────────────────────┤
│   最后 push 一个 "unpack stub" 帧     │
│   (给 unpack_frames() 用的临时帧)     │
├───────────────────────────────────────┤ ← 栈顶
```

关键点：
- **骨架帧是"外壳"**：只有栈空间的物理布局，locals/expression 里全是垃圾数据。
- **return address 全部是占位符** `deopt_entry(vtos, 0)`，只保证栈可遍历（is_interpreted_frame() 能识别）。
- **不是"加塞到 stub 和 caller 之间"**：老 stub 帧已被弹掉，caller 之上的所有编译内容全部覆盖为新骨架。
- **`frame_pcs[0] = deopt_sender.raw_pc()`（也就是 caller 里的 PC_caller）会被写到 outer 帧的 return address 位置**——等最老那个解释帧执行完 return 时，回到 caller。

## 六、Stage 4：填肉——unpack_frames → unpack_to_stack

deopt_blob 最后一步 call 到 `Deoptimization::unpack_frames`，它进入 `vframeArray::unpack_to_stack`。这时栈顶那个 stub 帧就是 `unpack_frame`。核心循环：

```cpp
// 先"锚定"每个 vframeArrayElement 对应的物理骨架帧
frame me = unpack_frame.sender(&map);  // = inner 骨架帧
for (index = 0; index < frames(); index++) {
  *element(index)->iframe() = me;       // element[0]=inner, element[1]=outer
  me = me.sender(&map);
}

// 再从老到新依次填充
frame* caller_frame = &me;  // 此时 me = caller
for (index = frames() - 1; index >= 0; index--) {  // 1, 0
  element(index)->unpack_on_stack(..., caller_frame, ...);
  caller_frame = element(index)->iframe();
}
```

对每个 element 调 `unpack_on_stack`：
1. **算好正确的 continuation pc**：用 method + bci 通过 `Interpreter::deopt_continue_after_entry / deopt_reexecute_entry` 查到解释器里"从这个 bci 继续跑"的入口地址。
2. **`Interpreter::layout_activation(...)`**：把骨架帧的 locals base、bcp、mdp、monitor block base、expression stack base 等指针字段填好——这一步让骨架变成真正合法的解释帧布局。
3. **`_frame.patch_pc(thread, pc)`**：把上一步算出的正确 pc 写到**内层骨架帧的 return address 槽位**（覆盖之前的占位符）。这就是 [vframeArray.cpp:303](/Users/liyang/workspace/jdk15/src/hotspot/share/runtime/vframeArray.cpp) 那句：
   ```cpp
   _frame.patch_pc(thread, pc);
   ```
4. **拷贝 locals / expression / monitor**：从 `vframeArrayElement` 里之前保存的 StackValueCollection 复制到骨架帧对应位置。

填完两轮后的最终栈：

```
├───────────────────────────────────────┤
│   caller() 帧                         │
├───────────────────────────────────────┤ ← 新增部分从这里开始
│   outer() 解释帧 ★已填肉              │
│                                       │
│   locals[]:      ← 从 vframeArray     │
│                    element(1) 拷贝    │
│   monitor block                       │
│   saved bcp = &outer_bytecode[bci_o]  │
│   saved locals ptr                    │
│   ...                                 │
│   expression stack ...                │
│   saved rbp                           │
│   return addr = PC_caller             │ ← frame_pcs[0]
├───────────────────────────────────────┤
│   inner() 解释帧 ★已填肉              │
│                                       │
│   locals[]:      ← 从 vframeArray     │
│                    element(0) 拷贝    │
│   monitor block                       │
│   saved bcp = &inner_bytecode[bci_i]  │
│   saved locals ptr                    │
│   ...                                 │
│   expression stack ...                │
│   saved rbp                           │
│   return addr = deopt_entry_for_outer │ ← patch_pc 写入的 continuation pc
│                                         (即 inner 返回时进入 outer 的 bci_o 后一条)
├───────────────────────────────────────┤
│   unpack stub 帧（临时）              │
│   return addr = deopt_entry(vtos, 0)  │ ← frame_pcs[number_of_frames] 填的
├───────────────────────────────────────┤ ← 栈顶
```

## 七、Stage 5：unpack_frames 返回 → 跳进解释器

unpack_frames 是 JRT_LEAF，返回后 deopt_blob 汇编做最后一步：**ret 掉那个 unpack stub 帧**，硬件从 stub 帧的 return address 弹出 pc = `deopt_entry(vtos, 0)`——这是解释器为 deopt 场景专门准备的入口，进入后：

1. 从栈上恢复 rbp、bcp、locals 指针等（就是刚才 layout_activation 填的字段）。
2. 从 `inner` 解释帧的 bcp 开始 fetch 下一条字节码继续执行。

至此，**执行流真正切换到解释器**，就在原本 outer 编译帧那片物理内存上跑 inner 的字节码。

## 八、老栈到新栈的对比图

```
=== deopt 之前 ===              === deopt 完成之后 ===

│ caller 帧              │      │ caller 帧              │
├───────────────────────┤      ├───────────────────────┤
│ outer(inline了inner)   │      │ outer 解释帧           │
│  编译帧（1 个物理帧）  │  →   │  (对应 inline 层 1)    │
│                        │      ├───────────────────────┤
├───────────────────────┤      │ inner 解释帧           │
│ safepoint stub 帧      │      │  (对应 inline 层 0)    │
├───────────────────────┤      ├───────────────────────┤
│ (deopt handler 中的    │      │ unpack stub 帧(临时,   │
│  各种临时 stub 帧)     │      │  即将 ret 到解释器)    │
                                └───────────────────────┘
```

## 九、回答你的核心问题

> "新生成的解释栈是加塞到 stub 帧和 caller 帧之间吗？"

**不是"加塞"，是"覆盖"**。准确的说法是：

1. **caller 帧完全不动**（顶多在其顶部按 `caller_adjustment` 向下扩几个字，作为最老那个解释帧 locals 的一部分）。
2. **被 deopt 的编译帧连同它上面的所有 stub 帧一起被物理弹掉**——rsp 一次性回到 caller 顶。
3. **在 caller 顶之上，按 inline 层数从老到新逐个 push 出多个骨架解释帧**（本例是 outer 解释帧、inner 解释帧）——这占用的物理内存和原来 outer 编译帧那片位置**大致重叠**（但大小不一样，通常解释帧总和 > 单个编译帧，因为一个编译帧同时承担了多个 inline 层的活）。
4. **最后再 push 一个临时 stub 帧**，让 unpack_frames 在里面填肉，填完再 ret 走。

所以正确的直觉是：**"原编译帧（1 帧） + 顶上的 stub 帧" 被替换为 "多个解释帧（N 帧，N = inline 深度） + 顶上一个临时 stub 帧"**，而 caller 帧作为整个替换过程的锚点，从头到尾都在原地不动。

一句话记忆：**"数据先跑路到 C heap（vframeArray），栈上先弹后建，最后回填并跳回解释器。"**
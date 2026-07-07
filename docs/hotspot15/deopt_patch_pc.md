
# 用帧栈图看 deopt patch 的全过程

## 一、场景假设

设想 Java 的一个调用链，都是 C2 编译过的代码：

```
main()  →  foo()  →  bar()  →  baz()
```

- `bar()` 里调用了 `baz()`，某个安全点上 `bar` 需要被 deopt。
- 这里 **`bar` 就是"被 patch 的帧"**（内层，因为它是我们关注的、被 deopt 的那一层）。
- **`baz` 更内**（栈顶方向）；**`foo` 更外**（栈底方向）。

x86_64 栈从高地址向低地址增长，我用 ↓ 表示地址变小的方向。

## 二、初始状态：还没 deopt 之前

```
高地址 (older / 外层)
┌─────────────────────────────────────────────┐
│           main() 的栈帧                     │
│  ...                                        │
├─────────────────────────────────────────────┤
│           foo() 的栈帧                      │
│  locals / spills ...                        │
│  saved rbp (foo)                            │
│  return addr → 回到 main 里 call foo 的下条 │
├─────────────────────────────────────────────┤ ← foo 的 sp
│           bar() 的栈帧    ★ 我们关注的帧    │
│                                             │
│  locals / spills ...                        │
│                                             │
│  ┌───────────────────────────────────────┐  │
│  │ orig_pc slot                          │  │  ← unextended_sp + _orig_pc_offset
│  │ (编译期在 bar 帧里预留的一个 slot)    │  │     初始值：垃圾/0，未使用
│  └───────────────────────────────────────┘  │
│                                             │
│  saved rbp (bar)                            │
│  return addr = 回 foo 的某条指令 A         │  ← ①**这里存的是"从 baz 返回后要执行的下条指令"**
├─────────────────────────────────────────────┤ ← bar 的 sp = &return_addr（往低走一格就是 baz 的 return addr）
│           baz() 的栈帧    (更内层)          │
│  ...                                        │
│  return addr = 回 bar 里 bar 的下条 = PC_B  │  ← ②**这就是"bar 里现在正执行到的位置"**
├─────────────────────────────────────────────┤ ← baz 的 sp
低地址 (newer / 内层，栈顶方向)
```

关键位置说明：

| 记号 | 语义 | 谁改它 |
|---|---|---|
| ① bar 帧里的 `return addr` | 硬件跳转用；表示 **bar 从更外的角度看回到哪** —— 但对我们没用，我们要 patch 的不是这个 |  |
| ② **baz 帧里的 `return addr`** | 硬件跳转用；**表示"baz 返回后 bar 要执行哪条指令"**——**这就是 bar 当前的 pc**，也是 patch_pc 的目标 | patch_pc 会改这里 |
| orig_pc slot | 编译期在 bar 帧里预留、专门给 deopt 用的备份槽位 | set_original_pc 会写这里 |

看代码就很清楚：

```cpp
// frame_x86.cpp: patch_pc 里
address* pc_addr = &(((address*) sp())[-1]);
```

`sp()` 是**当前 frame（bar）的 sp**，`sp()[-1]` 就是 **bar 的 sp 再往内一格**——正好落在 baz 帧的 return address 上，也就是图中 ②。这就是"bar 的 pc = 内一层帧的 return address"这个 x86 约定。

同时：

```cpp
address* nmethod::orig_pc_addr(const frame* fr) {
  return (address*) ((address)fr->unextended_sp() + _orig_pc_offset);
}
```

从 `unextended_sp` 往上偏一段，**落在 bar 自己帧内部**的一个专用 slot。

## 三、`cm->set_original_pc(this, pc())` 之后

只写了 orig_pc slot：

```
├─────────────────────────────────────────────┤
│           bar() 的栈帧                      │
│  locals / spills ...                        │
│                                             │
│  ┌───────────────────────────────────────┐  │
│  │ orig_pc slot = PC_B  ← ★★ 新写入 ★★  │  │  备份好业务 pc
│  └───────────────────────────────────────┘  │
│                                             │
│  saved rbp (bar)                            │
│  return addr = 回 foo 的某处            ①  │  没变
├─────────────────────────────────────────────┤
│           baz() 的栈帧                      │
│  ...                                        │
│  return addr = PC_B                     ②  │  没变
├─────────────────────────────────────────────┤
```

同时 C++ 侧的 `frame` 对象还是：`_pc = PC_B`。

## 四、`patch_pc(thread, deopt_handler)` 之后

进入 `patch_pc`：

```cpp
address* pc_addr = &(((address*) sp())[-1]);  // 指向 ②
*pc_addr = pc;                                 // 把 ② 改成 deopt_handler
address original_pc = CompiledMethod::get_deopt_original_pc(this);
```

先看 `get_deopt_original_pc(this)` —— 它内部检查 `is_deopt_pc(fr->pc())`，此刻 `fr->pc()` 读的是 `frame::_pc` 字段（不是从栈上现读！），仍是 **PC_B**，**不是** deopt handler → 返回 `NULL` → 走 else：`_pc = deopt_handler`。

栈帧图：

```
├─────────────────────────────────────────────┤
│           bar() 的栈帧                      │
│  locals ...                                 │
│  ┌───────────────────────────────────────┐  │
│  │ orig_pc slot = PC_B                   │  │  保留业务 pc
│  └───────────────────────────────────────┘  │
│  saved rbp                                  │
│  return addr = 回 foo                   ①  │
├─────────────────────────────────────────────┤
│           baz() 的栈帧                      │
│  ...                                        │
│  return addr = deopt_handler   ★★ patch ★★ │  ②
├─────────────────────────────────────────────┤
```

C++ 侧对象状态：

```
frame 对象 (bar):
  _pc          = deopt_handler        ← 更新了
  _deopt_state = not_deoptimized       ← 走的是 else 分支
```

到这里就实现了：**baz 一 ret，硬件就跳到 deopt_handler**。

## 五、稍后：栈遍历时又构造出一个新的 `frame` 对象来看 bar

比如 GC 或 vframe 展开来遍历栈，走到 bar 时会用 `sender()` 之类构造一个新的 `frame`。看 [frame_x86.inline.hpp:53](/Users/liyang/workspace/jdk15/src/hotspot/cpu/x86/frame_x86.inline.hpp) 附近的构造函数：

```cpp
// 简化伪代码
frame(sp, unextended_sp, ...) {
  _pc = *(sp - 1);                                 // 从栈上读，此时 = deopt_handler
  address original_pc = get_deopt_original_pc(this);
  if (original_pc != NULL) {                       // deopt_handler → is_deopt_pc = true
    _pc = original_pc;                             // ★ 把 _pc 恢复回 PC_B
    _deopt_state = is_deoptimized;
  } else {
    _deopt_state = not_deoptimized;
  }
}
```

新构造的 frame 对象状态：

```
frame 对象 (bar) [重新构造]:
  _pc          = PC_B                  ← 从 orig_pc slot 恢复
  _deopt_state = is_deoptimized
```

这样 JVM 后续用这个 frame 去查 scope descriptor / OopMap，一切都用**业务 pc = PC_B** 索引，能查到正确的调试信息、oop 位置。

## 六、若此时又对这个"新构造的 frame"调 patch_pc（例如 vframeArray 恢复流程）

进入 `patch_pc`：

```cpp
*pc_addr = pc;                                    // ② 又改一次栈上返回地址
address original_pc = get_deopt_original_pc(this);// _pc = PC_B, is_deopt_pc(PC_B)=false → NULL
_deopt_state = not_deoptimized;
_pc = pc;
```

咦？这也是走 else……那 if 分支到底啥时候进？

if 分支进入的时刻是：**同一个 frame 对象在两次连续 patch_pc 之间没被重建**。看 `deoptimize()`：

```cpp
cm->set_original_pc(this, pc());   // 写 orig_pc slot = PC_B
patch_pc(thread, deopt);           // 内部 else 分支，_pc: PC_B → deopt
                                   // 之后 this 对象仍然存活
```

如果紧接着又对**这个还活着的 `this`** 再调一次 `patch_pc`（比如中间流程有第二次 patch），那时 `_pc = deopt_handler`，`is_deopt_pc(_pc) = true`，就走 if 分支保持 `_pc` 不动。或者，别的调用点先手动做过 `_pc = deopt_handler` 的操作、再进来的场景。

总之 if 分支的意义就是幂等：**如果这个 `frame` C++ 对象已经处于 "已 deopt" 视图，那再 patch 一次也不要再改 `_pc` 了，只更新 `_deopt_state` 状态标记。**

## 七、把三个 pc 的角色画在一张对照表里

```
                         ┌──────────────┬──────────────┬──────────────┐
                         │   ② 栈上     │  orig_pc     │  frame 对象  │
                         │  (baz的      │   slot       │   ::_pc      │
                         │  return addr)│  (bar 帧内)  │  (C++ 内存)  │
─────────────────────────┼──────────────┼──────────────┼──────────────┤
初始                     │  PC_B        │   -          │  PC_B        │
set_original_pc 后       │  PC_B        │  PC_B        │  PC_B        │
patch_pc 后              │ deopt_handler│  PC_B        │ deopt_handler│
——— baz ret 触发跳转 ——— (硬件跳到 deopt_handler)
遍历栈重建 frame 后      │ deopt_handler│  PC_B        │  PC_B        │  ← _pc 靠 orig_pc slot 恢复
                         │  硬件用      │  永久备份    │  JVM 逻辑用  │
                         └──────────────┴──────────────┴──────────────┘
```


[任务规划完成]
这个问题问到了 deopt 的一个非常本质的前提。答案是：**HotSpot 从不对"正在运行、栈顶正在执行"的最内层帧做 deopt**——所以 `sp()[-1]` 不会踩空，它总是能落到一个真实存在的、比 baz 更内层的东西上。下面详细展开。

## 一、先纠正一个隐含假设

"最内层函数 baz" 这个词有歧义，得分两种情况：

### 情况 A：baz 是当前 CPU 正在执行指令的那个函数（rip 就在 baz 的代码里）

这个时候 baz 根本**不会**作为 deopt 的对象出现在 `frame::deoptimize()` 里。理由：

1. 触发 deopt 的时机都是安全点（safepoint poll、uncommon trap、class unloading 等），在这些时机上：
    - 如果是 baz 自己触发的 uncommon trap，deopt 走的是完全不同的路径（`Deoptimization::uncommon_trap` → 直接构造 vframeArray），**不走 `frame::deoptimize()` + `patch_pc`**。
    - 如果是外部（VM 线程）要 deopt 某个方法，被 deopt 的目标是**返回后还要继续执行编译代码的那些帧**，也就是 baz 更外层的、当前"卡在一个 call 指令之后"的帧。
2. `frame::deoptimize()` 里那句：
   ```cpp
   address deopt = cm->is_method_handle_return(pc()) ?
                   cm->deopt_mh_handler_begin() :
                   cm->deopt_handler_begin();
   ```
   本身就假设 `pc()` 是"一条 call 指令的下一条"（return address 语义）。栈顶正在跑的帧的 pc 是"当前正在执行的指令"，不是 return address 语义，不适用这套逻辑。

### 情况 B：baz 是"最内的 Java 编译帧"，但它下面还有 native / VM runtime 的 C++ 栈帧

这才是真实场景。触发 deopt 时线程一定是**从 Java 陷入了 VM**（走 safepoint stub、走 runtime call），此刻栈顶已经是 VM 的 C++ 栈帧、或者是一个 stub 生成的调用桩帧。也就是说 baz **不是**物理意义上的栈顶。

## 二、真实栈顶长这样

以典型 safepoint 触发路径为例（safepoint poll → 陷入 VM）：

```
高地址（外层）
├─────────────────────────────────┤
│      foo() 编译帧               │
├─────────────────────────────────┤
│      bar() 编译帧               │
├─────────────────────────────────┤
│      baz() 编译帧    ★"最内 Java帧"
│  locals / spills                │
│  ┌───────────────────────────┐  │
│  │ orig_pc slot (baz 自己的) │  │
│  └───────────────────────────┘  │
│  saved rbp                      │
│  return addr = 回 bar 的 PC_bar │
├─────────────────────────────────┤ ← baz 的 sp
│  safepoint blob / stub 栈帧    │  ← 由 SharedRuntime 生成的调用桩
│  save 所有 caller-saved regs   │
│  ...                            │
│  return addr = **PC_baz**       │  ← ②baz 里 safepoint poll 那条指令的下一条
├─────────────────────────────────┤
│  VM runtime C++ 栈帧            │  ← SafepointSynchronize::block 等
│  ...                            │
├─────────────────────────────────┤
低地址（栈顶，rsp 现在在这里）
```

关键点：
- **baz 自己不在栈顶**。它下面还压着 stub 帧 + VM C++ 帧。
- **baz 的"pc"（也就是它被打断时正准备执行的下一条指令 PC_baz）此刻正好保存在 stub 栈帧的 return address 位置**——因为对 baz 来讲，safepoint poll 就是一次"call 到 stub"，stub 帧的 return address 就是 baz 的 pc。
- 所以 baz 作为一个 `frame` 对象，它的 `sp()` 就是 stub 帧最顶端那个 return address 槽的**上方一格**；`sp()[-1]` 落到的正是 **PC_baz 这个真实存在的 return address 槽**。

对着代码看：

```cpp
// frame_x86.cpp
address* pc_addr = &(((address*) sp())[-1]);
```

这里的 `sp()` 是 **baz 帧**的 sp，`sp()[-1]` 就是"内一层帧（stub 帧）最顶上的那个 return address"，它一定是有效的、真实分配的栈内存。

## 三、如果真出现"没有内层帧"会怎样

理论上如果 baz 没有内层（也就是 rip 真的在 baz 里），那 `sp()[-1]` 会踩到**尚未分配的栈空间**（rsp 以下），读写都是 UB。但这在 HotSpot 里被下面几条保证共同排除掉了：

1. **can_be_deoptimized() 的检查**：
   ```cpp
   bool frame::can_be_deoptimized() const {
     if (!is_compiled_frame()) return false;
     ...
     return !nm->is_at_poll_return(pc());
   }
   ```
   这里 `pc()` 必须是这个帧的一个"有效返回点/调用点"（scope 表里能查到）。栈顶正在执行的帧的 rip 通常不在这些点上。

2. **调用 `frame::deoptimize()` 的所有 caller**都是走的"栈遍历" —— 从当前线程的 `last_Java_frame` 开始 `sender()` 一层层往外找编译帧。既然是"往外"，那被找到的帧一定不是栈顶帧，一定有更内的一层。

3. **触发 deopt 的时机本身**（前面讲的）保证了线程已经陷入 VM，栈顶是 VM 帧。

4. `pc_addr` 计算依赖的是 **"我的 pc = 内层帧的 return address"** 这个 x86 调用约定。这个约定只对"曾经调用出去、现在等着被返回"的帧成立——**栈顶正在执行**的帧根本没有对应的 return address 存在栈上（它的 pc 在 rip 寄存器里，而不在内存里）。所以 HotSpot 才小心地避开对这种帧调用 `patch_pc`。

## 四、一句话总结

`sp()[-1]` 之所以永远有意义，是因为：

**能进入 `frame::deoptimize()` / `frame::patch_pc()` 的那个"编译帧"，一定不是物理栈顶帧——它下面一定还压着更内层的东西（另一个 Java 帧、safepoint stub 帧、或 VM C++ 帧），而那个更内层帧最顶上的 return address 槽，就是当前帧 pc 的物理存储位置。**

换句话说，HotSpot 只对"处于 call 之后、正等 callee 返回"的帧做 patch；对"rip 就在自己代码里跑着"的栈顶帧从不走这条路径——那种情况要么根本不 deopt（等下次到达一个 call/poll 点再说），要么走 uncommon_trap 直接构造 vframeArray，压根不需要动栈上的 pc。
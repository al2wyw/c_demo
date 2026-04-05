
## 退优化后进入解释器，**可以**再次通过 OSR 进入编译代码

整个流程是一个完整的自适应优化循环，下面详细说明。

### 完整的循环流程

```
编译代码执行
    │
    │ uncommon_trap 触发
    ▼
uncommon_trap_inner()
    ├── 更新 MDO trap 历史（inc_trap_count, set_trap_state）
    ├── 根据 action 决定：
    │     Action_reinterpret → make_not_entrant = true, reprofile = true
    │     Action_make_not_entrant → make_not_entrant = true
    │     Action_none → 继续用旧代码
    │
    ├── nm->make_not_entrant()  ← 让旧编译代码失效
    └── CompilationPolicy::reprofile()  ← 重置调用计数器起始值
            │
            └── mdo->reset_start_counters()
                  仅重置 _invocation_counter_start / _backedge_counter_start
                  （不清除 profile data！）
    │
    ▼
fetch_unroll_info_helper() → 构建解释器帧
    │
    ▼
解释器继续执行（从 trap 的 BCI 处重新执行）
    │
    │ 每次 backward branch（循环回边）
    ▼
backedge_counter 累加
    │
    │ 超过阈值
    ▼
InterpreterRuntime::frequency_counter_overflow(branch_bcp)
    │
    └── CompilationPolicy::policy()->event(method, bci, CompLevel_none, ...)
              │
              └── TieredThresholdPolicy::method_back_branch_event()
                        │
                        └── compile(imh, bci, next_osr_level, thread)
                              ← 触发 OSR 编译（带有更新后的 MDO 信息）
    │
    │ OSR nmethod 编译完成后返回
    ▼
frequency_counter_overflow 返回 osr_nm（非 NULL）
    │
    ▼
解释器汇编代码检测到返回值非 NULL：
    ── SharedRuntime::OSR_migration_begin()  ← 迁移局部变量和 monitor
    ── pop 解释器帧
    ── jmp [rbx + osr_entry_point_offset]   ← 直接跳入 OSR 编译代码！
```

### 关键代码证据

**① 解释器 backedge 触发 OSR（x86 汇编）**

```cpp
// templateTable_x86.cpp
__ call_VM(noreg,
           CAST_FROM_FN_PTR(address, InterpreterRuntime::frequency_counter_overflow),
           rdx);  // rdx = branch_bcp

// rax: osr nmethod (osr ok) or NULL (osr not possible)
__ testptr(rax, rax);
__ jcc(Assembler::zero, dispatch);   // NULL → 继续解释执行
__ cmpb(Address(rax, nmethod::state_offset()), nmethod::in_use);
__ jcc(Assembler::notEqual, dispatch);

// 有效的 OSR nmethod → 迁移并跳入
__ mov(rbx, rax);
call_VM(..., SharedRuntime::OSR_migration_begin);
__ jmp(Address(rbx, nmethod::osr_entry_point_offset()));  // ← 跳入编译代码
```

**② OSR 编译由 `method_back_branch_event` 触发**

```cpp
// tieredThresholdPolicy.cpp
CompLevel next_osr_level = loop_event(imh, level, thread);
if (!CompileBroker::compilation_is_in_queue(imh) && (next_osr_level != level)) {
    compile(imh, bci, next_osr_level, thread);  // 触发 OSR 编译
}
```

### 与普通重编译的区别

| 场景 | 触发方式 | 入口 |
|---|---|---|
| 方法调用触发重编译 | `invocation_counter` 溢出 | 方法入口（`InvocationEntryBci`） |
| **OSR（栈上替换）** | `backedge_counter` 溢出 | **循环回边处**（`branch_bci`） |

OSR 的特殊之处在于：它不需要等到方法返回再重新调用编译版本，而是**在解释器正在执行循环的过程中**，直接把当前栈帧"迁移"到编译代码的栈帧，从循环的某个 BCI 处无缝切入编译代码继续执行。

### 防止无限循环的保护机制

如果同一个 BCI 反复 deopt → recompile → deopt，`uncommon_trap_inner` 中有多重保护：

1. **`PerBytecodeTrapLimit`**：同一 BCI trap 次数超限 → 强制 `make_not_entrant`
2. **`PerBytecodeRecompilationCutoff`**：同一 BCI 重编译次数超限 → `inc_overflow_recompile_count`，最终 `make_not_compilable`
3. **`PerMethodRecompilationCutoff`**：整个方法重编译次数超限 → `set_not_compilable`，彻底放弃编译，永远在解释器中执行
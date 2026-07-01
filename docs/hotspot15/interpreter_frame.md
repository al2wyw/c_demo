### 1. `sp`：真实的运行时栈顶
- 解释器执行字节码时，每条 `iadd / aload / invokeXXX` 都会改它。

### 2. `fp`（rbp）：帧基址
- 由 `enter` 指令建立：`push rbp; mov rsp, rbp`。
- 一旦确立，整个方法执行期间不再改变（除非进入 native 调用栈布局）。
- 所有定长 slot 用 `frame::interpreter_frame_xxx_offset` 表通过 `[rbp + offset*word]` 寻址

### 3. `sender_sp`（`sender_sp_offset = 2`）：真实的"调用者 sp"，即 call 指令之前的栈顶位置，仅限非解释帧使用
- call指令会把`return pc`入栈，所以`sender_sp`也就是 `rbp + 2` 紧挨在 `return pc` 上面那一slot。
- 用途：所有 *非解释器* 帧（C2 编译帧、stub）都用它作为 caller sp。注释里写的 "non-interpreter frames" 即指此。

### 4. `interpreter_frame_sender_sp`（offset = -1）：caller 未扩展前的 sp(分配完callee参数空间后???)，即sender_unextended_sp
- 用途：解释器和适配器（adapter）会把"参数"留在 caller 的栈顶并复用为 callee 的 locals。这意味着 callee 看到的 `sender_sp`（`rbp+2`）落在被扩展过的区域，而不是 caller "原始" 的 sp 边界。
- `set_interpreter_frame_sender_sp` 在 layout_activation 重布局 deopt 帧时会重新写它（`extra_locals != 0` 的修正分支）。
- 在 [frame_x86.cpp:121](/jdk15/src/hotspot/cpu/x86/frame_x86.cpp)，构造 sender frame 时也是从这里取出 caller 的 unextended sp：
  ```
  sender_unextended_sp = (intptr_t*) this->fp()[interpreter_frame_sender_sp_offset];
  ```

### 5. `last_sp`（offset = -2）：调用其他函数时当前栈的 rsp (分配完callee参数空间后,参考invokexxxx对应的字节码)
- 语义来自 [frame_x86.inline.hpp:191](/jdk15/src/hotspot/cpu/x86/frame_x86.inline.hpp) 的 `interpreter_frame_tos_address()`：
  ```cpp
  intptr_t* last_sp = interpreter_frame_last_sp();
  if (last_sp == NULL) {
      return sp();          // 没在调用别人，esp 自身就是表达式栈顶
  } else {
      return last_sp;       // 正在调用别人，esp 已经被 callee 占用，
                            // 真正的表达式栈顶在 last_sp 处
  }
  ```
- 用途：解释器是寄存器约定式地用 rsp 当 esp。当本方法 `invoke` 调用别的方法时，rsp 会继续向下增长以容纳 callee 帧；如果 GC 触发或者发生 deopt，VM 必须知道"调用前我自己的表达式栈顶在哪里"才能正确扫描 oop / 重建帧——这就是 last_sp 的存在意义。

### 6. `unextended_sp`：未扩展前的 sp，call_stub和c2i adapter进入解释帧时设置的unextended_sp还不一样???
- 取自 [frame_x86.hpp:113-118](/jdk15/src/hotspot/cpu/x86/frame_x86.hpp) 的注释（这段是关键定义）：
  > The interpreter and adapters will extend the frame of the caller. Since oopMaps are based on the sp of the caller before extension we need to know that value. However in order to compute the address of the return address we need the real "raw" sp.
- 翻译过来就是两个观察点：
  1. 解释器/适配器会"扩展" caller 的栈（用 caller 的栈顶来放 callee 的实参/locals）。
  2. OopMap 是按 caller 在被扩展之前的 sp 生成的，所以遍历 oop 时用 `unextended_sp` 才正确；而要算 `return pc` 这种与"被扩展后的真实 sp"挂钩的东西，又得用 `sp`。
- 构造时机：
  - 单参构造（[frame_x86.inline.hpp:46](/jdk15/src/hotspot/cpu/x86/frame_x86.inline.hpp)）：`_unextended_sp = sp`，假定无扩展。
  - 三参构造（[frame_x86.inline.hpp:66](/jdk15/src/hotspot/cpu/x86/frame_x86.inline.hpp)）：调用方显式传入，比如 `sender_for_interpreter_frame` 里：
    ```
    intptr_t* unextended_sp = interpreter_frame_sender_sp();   // caller 的 -1 槽
    return frame(sender_sp, unextended_sp, link(), sender_pc());
    ```
  - 之后还会调用 `adjust_unextended_sp()`（[frame_x86.cpp:372](/jdk15/src/hotspot/cpu/x86/frame_x86.cpp)）针对 deopt 场景再做修正。

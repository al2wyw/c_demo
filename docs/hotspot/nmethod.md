[任务规划完成]
[读取文件]
现在我有了完整的 `nmethod.hpp` 内容，可以给出全面详细的解释。

---

## nmethod 内存布局及各组件功能详解

### 一、nmethod 的定位

`nmethod`（native method）是 JIT 编译后的 Java 方法在内存中的完整表示，继承自 `CodeBlob`。它不仅包含机器码，还包含 GC、调试、异常处理、去优化等所有运行时所需的元数据。

---

### 二、完整内存布局

```
nmethod 内存（连续分配在 CodeCache 中）
┌──────────────────────────────────────────────────────┐  ← header_begin()
│  [Header]  nmethod 对象本身（C++ 结构体）              │
│   - _method, _entry_point, _verified_entry_point     │
│   - _osr_entry_point, _compile_id, _comp_level       │
│   - 所有 _xxx_offset 字段（各区域的偏移量索引）        │
│   - _exception_cache, _pc_desc_cache                 │
├──────────────────────────────────────────────────────┤  ← consts_begin()
│  [Consts]  常量区                                     │
│   - double/long/float 字面量                          │
│   - 编译时确定的常量池数据                             │
├──────────────────────────────────────────────────────┤  ← insts_begin() / code_offset()
│  [Code Body]  主体机器码                              │
│   - inline cache check（类型检查桩）                  │
│   - verified entry point（跳过类型检查的入口）         │
│   - 实际编译的 Java 字节码对应的机器指令               │
├──────────────────────────────────────────────────────┤  ← exception_begin()
│  [Exception Handler]  异常处理入口                    │
│   - 编译帧的异常分发入口（跳转到 exception_blob）      │
├──────────────────────────────────────────────────────┤  ← deopt_handler_begin()
│  [Deopt Handler]  去优化处理入口                      │
│   - 普通 deopt 入口（跳转到 DeoptimizationBlob）      │
├──────────────────────────────────────────────────────┤  ← deopt_mh_handler_begin()
│  [Deopt MH Handler]  MethodHandle 去优化入口          │
├──────────────────────────────────────────────────────┤  ← stub_begin()
│  [Stub Code]  桩代码区                                │
│   - 各种内联桩（如 safepoint poll、monitor enter 等） │
├──────────────────────────────────────────────────────┤  ← oops_begin()
│  [OopTable]  嵌入式 oop 表                            │
│   - 编译代码中引用的 Java 对象（oop）                  │
│   - 索引从 1 开始（0 保留为 null）                    │
├──────────────────────────────────────────────────────┤  ← metadata_begin()
│  [MetadataTable]  元数据表                            │
│   - 编译代码中引用的 Method*、Klass* 等元数据          │
│   - 索引从 1 开始（0 保留为 null）                    │
├──────────────────────────────────────────────────────┤  ← scopes_data_begin()
│  [ScopesData]  调试信息数据区                         │
│   - 序列化的 ScopeDesc 链（方法 + bci + 局部变量）     │
│   - 被 PcDesc.scope_decode_offset 索引               │
├──────────────────────────────────────────────────────┤  ← scopes_pcs_begin()
│  [ScopesPcs]  PcDesc 数组                            │
│   - PcDesc[] 按 pc_offset 升序排列                   │
│   - 每条记录：pc_offset → scope_decode_offset        │
│   - 首尾各有一个 sentinel 哨兵条目                    │
├──────────────────────────────────────────────────────┤  ← dependencies_begin()
│  [Dependencies]  编译依赖表                           │
│   - 记录编译时假设的类层次关系（CHA 假设）             │
│   - 类被重定义/加载新子类时触发 deopt                  │
├──────────────────────────────────────────────────────┤  ← handler_table_begin()
│  [ExceptionHandlerTable]  异常处理表                  │
│   - pc → handler_pc 的映射                           │
│   - 对应 Java 字节码的 try-catch 块                   │
├──────────────────────────────────────────────────────┤  ← nul_chk_table_begin()
│  [ImplicitExceptionTable]  隐式空指针异常表            │
│   - 记录可能触发 SIGSEGV 的 PC 地址                   │
│   - 将信号转换为 NullPointerException                 │
└──────────────────────────────────────────────────────┘  ← nul_chk_table_end() / nmethod_end
```

---

### 三、各区域详细说明

#### 1. Header（nmethod 对象本身）

nmethod 的 C++ 对象头部，存储所有元信息：

| 字段 | 类型 | 说明 |
|------|------|------|
| `_method` | `Method*` | 对应的 Java 方法 |
| `_entry_bci` | `int` | OSR 方法的入口 bci，普通方法为 `InvocationEntryBci` |
| `_entry_point` | `address` | 带类型检查的普通入口（inline cache check 之前） |
| `_verified_entry_point` | `address` | 跳过类型检查的入口（接收者类型已验证） |
| `_osr_entry_point` | `address` | OSR（栈上替换）入口 |
| `_compile_id` | `int` | 编译任务 ID |
| `_comp_level` | `int` | 编译层级（C1=1/2/3，C2=4） |
| `_state` | `unsigned char` | 生命周期状态（见下） |
| `_hotness_counter` | `int` | 热度计数器，NMethodSweeper 用于决定是否回收 |
| `_exception_cache` | `ExceptionCache*` | 异常处理缓存链表头 |
| `_pc_desc_cache` | `PcDescCache` | 4槽热点 PcDesc 缓存 |

**nmethod 生命周期状态**：

```
in_use ──→ not_entrant ──→ zombie ──→ (flush/purge)
                                ↑
                           unloaded ──┘
```

| 状态 | 含义 |
|------|------|
| `in_use` | 正常可执行 |
| `not_entrant` | 已标记去优化，不接受新调用，但已有活跃帧可继续执行 |
| `zombie` | 无活跃帧，等待 Sweeper 回收 |
| `unloaded` | 类已卸载，立即转为 zombie |

---

#### 2. Consts（常量区）

存放编译时确定的数值常量（`double`、`long`、`float`），这些常量在机器码中通过 PC 相对寻址引用，避免了立即数编码的限制。

---

#### 3. Code Body（主体机器码）

```
insts_begin()
    │
    ├── [inline cache check]       ← entry_point 指向这里
    │     mov rax, [receiver.klass]
    │     cmp rax, expected_klass
    │     jne ic_miss_stub
    │
    ├── [verified entry point]     ← verified_entry_point 指向这里
    │     push rbp
    │     sub rsp, frame_size
    │     ... 编译后的 Java 逻辑 ...
    │
    └── [return / tail calls]
insts_end() = stub_begin()
```

- `entry_point`：调用方不确定接收者类型时使用，包含 inline cache 检查
- `verified_entry_point`：调用方已确认类型（如静态调用、特化调用），直接跳过检查

---

#### 4. Stub Code（桩代码区）

内联在 nmethod 中的小段桩代码，包括：
- **Safepoint poll stub**：安全点轮询失败时的处理跳转
- **Monitor enter/exit stub**：synchronized 块的锁操作
- **Exception handler stub**：`exception_begin()` 指向此处，负责将异常分发到 `OptoRuntime::handle_exception_C`
- **Deopt handler stub**：`deopt_handler_begin()` 指向此处，负责跳转到 `DeoptimizationBlob`

---

#### 5. OopTable（嵌入式 oop 表）

```cpp
oop*  oops_begin() const { return (oop*)(header_begin() + _oops_offset); }
oop   oop_at(int index)  { return index == 0 ? NULL : *oop_addr_at(index); }
```

- 存储编译代码中引用的 Java 对象（如字符串常量、Class 对象）
- 索引从 **1** 开始（0 保留为 null）
- GC 扫描时通过 `oops_do()` 遍历此表，更新对象引用
- relocation info 中的 `oop_type` 记录机器码中引用此表的位置

---

#### 6. MetadataTable（元数据表）

```cpp
Metadata** metadata_begin() const { return (Metadata**)(header_begin() + _metadata_offset); }
```

- 存储编译代码中引用的 `Method*`、`Klass*` 等元数据
- 与 OopTable 类似，索引从 1 开始
- 类卸载时通过此表检测 nmethod 是否需要失效

---

#### 7. ScopesData（调试信息数据区）

序列化存储的 `ScopeDesc` 链，每个 PcDesc 通过 `scope_decode_offset` 索引到此区域：

```
scopes_data_begin()
    │
    ├── [ScopeDesc for PC_1]  (内联方法 A, bci=5)
    │     method_index, bci, locals[], expressions[], monitors[]
    │     sender_decode_offset → 下一层
    │
    ├── [ScopeDesc for PC_1 outer]  (方法 B, bci=42)
    │     ...
    │
    ├── [ScopeDesc for PC_2]
    │     ...
    └── ...
scopes_data_end()
```

用途：异常堆栈填充、JVMTI 局部变量查询、Deoptimization 帧重建。

---

#### 8. ScopesPcs（PcDesc 数组）

```cpp
PcDesc* scopes_pcs_begin() const { return (PcDesc*)(header_begin() + _scopes_pcs_offset); }
```

- 按 `pc_offset` **升序**排列的 `PcDesc[]` 数组
- 首尾各有一个哨兵（sentinel）条目
- 每条 PcDesc：`pc_offset` → `scope_decode_offset`（指向 ScopesData）
- 查找算法：热点缓存 → 准二分搜索 → 线性扫描

---

#### 9. Dependencies（编译依赖表）

```cpp
address dependencies_begin() const { return header_begin() + _dependencies_offset; }
```

记录编译时的 **CHA（类层次分析）假设**，例如：
- "类 A 没有子类"
- "方法 B 没有被覆写"

当新类加载或类被重定义时，JVM 检查这些依赖，若失效则触发 nmethod 去优化（`make_not_entrant`）。

---

#### 10. ExceptionHandlerTable（异常处理表）

```cpp
address handler_table_begin() const { return header_begin() + _handler_table_offset; }
```

对应 Java 字节码的 `try-catch` 块，格式为：

```
[throw_pc_offset] → [handler_pc_offset, exception_klass_index]
```

异常发生时，通过 `handler_for_exception_and_pc()` 查找对应的 catch 块入口。同时配合 `ExceptionCache`（链表缓存）加速重复异常的处理。

---

#### 11. ImplicitExceptionTable（隐式空指针异常表）

```cpp
address nul_chk_table_begin() const { return header_begin() + _nul_chk_table_offset; }
```

记录可能触发 `SIGSEGV` / `SIGBUS` 的 PC 地址，格式为：

```
[fault_pc_offset] → [continuation_pc_offset]
```

当 CPU 访问 null 指针触发信号时，信号处理器查此表：
- 找到 → 将 PC 修改为 `continuation_pc`（通常是抛出 NPE 的桩），继续执行
- 找不到 → 真正的段错误，JVM crash

---

### 四、三个入口点的关系

```
调用方（invokevirtual）
    │
    ├── 未内联/类型未知 ──→ entry_point
    │                          │
    │                    [inline cache check]
    │                    klass == expected?
    │                    ├── 是 ──→ verified_entry_point
    │                    └── 否 ──→ ic_miss → 重新解析
    │
    ├── 类型已知（特化）──→ verified_entry_point（直接跳过检查）
    │
    └── OSR 替换        ──→ osr_entry_point（从解释器切换到编译帧）
```

---
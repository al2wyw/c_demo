

Relocation 区域位于 **Header 之后、Consts 之前**，完整布局应该是：

```
┌──────────────────────────────────────────────────────┐  ← header_begin()
│  [Header]  nmethod 对象本身（C++ 结构体）              │
├──────────────────────────────────────────────────────┤  ← relocation_begin()
│  [Relocation]  重定位信息                             │
│   - 描述机器码中所有需要"打补丁"的位置                 │
│   - 每条记录：位置 + 类型（oop/metadata/call/...）    │
├──────────────────────────────────────────────────────┤  ← consts_begin()
│  [Consts]  常量区                                     │
│   ...                                                │
```

---

### Relocation 区域详解

Relocation（重定位信息）是 nmethod 中非常关键的一块区域，它记录了机器码中所有**需要在运行时动态修正或扫描的位置**。

#### 存储结构

Relocation 信息以紧凑的流式格式存储，由 `RelocIterator` 负责遍历解析，定义在 [relocInfo.hpp](/Users/liyang/IdeaProjects/jdk8u/hotspot/src/share/vm/code/relocInfo.hpp) 中：

```cpp
// 每条 relocInfo 由两部分组成：
// - type (4 bits)：重定位类型
// - offset (12 bits)：相对上一条记录的 PC 偏移量（单位：字）
// 如果偏移量超过 12 bits，则插入额外的 prefix 记录
```

#### 重定位类型（`relocInfo::relocType`）

| 类型 | 说明 |
|------|------|
| `oop_type` | 指向 OopTable 中某个 oop 的引用，GC 时需要更新 |
| `metadata_type` | 指向 MetadataTable 中 `Klass*`/`Method*` 的引用 |
| `virtual_call_type` | 虚方法调用点（含 inline cache），需要支持 IC 清理/修补 |
| `opt_virtual_call_type` | 优化后的虚调用（已特化，但仍可 deopt） |
| `static_call_type` | 静态调用点，目标地址可能需要修补 |
| `runtime_call_type` | 调用 C++ 运行时函数的调用点 |
| `external_word_type` | 指向 JVM 内部某个固定地址的引用 |
| `internal_word_type` | 指向 nmethod 内部某个位置的引用（如跳转目标） |
| `section_word_type` | 跨 section 的内部引用 |
| `poll_type` | safepoint 轮询点（读操作） |
| `poll_return_type` | 方法返回处的 safepoint 轮询点 |

#### 主要用途

1. **GC 扫描**：通过遍历 `oop_type` 和 `metadata_type` 记录，找到机器码中所有嵌入的对象引用，进行标记或更新
2. **Inline Cache 维护**：通过 `virtual_call_type` 记录定位所有虚调用点，在类加载/卸载时清理或修补 IC
3. **Safepoint 检测**：通过 `poll_type` / `poll_return_type` 记录，快速判断某个 PC 是否是安全点轮询位置（`is_at_poll_return()`、`is_at_poll_or_poll_return()` 就是通过遍历 reloc 实现的）
4. **代码修补**：调用点目标地址变化时（如方法被重新编译），通过 `static_call_type` 记录找到所有调用点并更新目标地址

#### 与 OopTable/MetadataTable 的关系

```
机器码中的指令
    mov rax, [rip + offset]   ← relocInfo 记录此处是 oop_type，index=3
                                          ↓
                                    oops_begin()[3-1]  ← OopTable 中第3个槽
                                          ↓
                                    实际的 Java 对象地址
```

Relocation 记录的是**位置**（在机器码中的偏移），OopTable/MetadataTable 存储的是**值**（实际的对象/元数据指针）。两者配合，GC 才能既找到位置又能更新值。
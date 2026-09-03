## 先厘清 nm 看到的和 dlsym 能拿到的**不是同一集合**

**核心事实**：`nm` 和 `dlsym` 查的是**不同的表**：

| 工具 | 查询范围 | 内容 |
|---|---|---|
| **`nm libjvm.dylib`** | 完整符号表（symbol table）—— `__symtab` 段 | 所有编译期产生的符号，含 `local`、`weak`、`private extern`、`hidden` 等 |
| **`nm -g libjvm.dylib`** | 全局符号 | 编译器标记为 `.globl` 的 |
| **`nm -Ug libjvm.dylib`** | 外部可见符号（`external` 且 `defined`）—— 大致对应 `-exported_symbols_list` 里的 | 真正对外可见 |
| **`dlsym(handle, name)`** | **动态符号表** —— `__DATA,__la_symbol_ptr` + `__DATA,__got` 通过 `LC_DYSYMTAB` 中的 `iextdefsym..nextdefsym` 范围能索引到的 | 仅**导出的**外部符号 `external` |


## 立刻验证这个假设

```bash
LIBJVM=$JAVA_HOME/lib/server/libjvm.dylib   # 或你实际用的路径

# 1. nm 完整表看到的（你已经确认能找到）
nm "$LIBJVM" 2>&1 | grep JvmtiEnvBase17_head_environment

# 2. nm 只看"外部+已定义"的符号 —— dlsym 只能找到这里的
nm -Ug "$LIBJVM" 2>&1 | grep JvmtiEnvBase17_head_environment

# 3. nm 每列都带类型标志 —— 关键看第 2 列的字母
nm -m "$LIBJVM" 2>&1 | grep JvmtiEnvBase17_head_environment
```

**MachO格式的符号会在ELF格式的符号加前缀"_"**
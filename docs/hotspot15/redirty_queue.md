
## G1RedirtyCardsQueue 的作用

`G1RedirtyCardsQueue` 是 G1 GC 中用于**收集需要重新标脏（Redirty）的卡表条目**的工作线程本地队列。

---

### 核心职责

在 G1 疏散（Evacuation）过程中，对象被从一个 Region 复制到另一个 Region。如果某个对象被复制到了**老年代 Region**，那么指向该对象的引用所在的卡（Card）需要被**重新标脏**，以便后续的 Refinement 线程能够重新处理这些跨代引用，维护 RemSet 的正确性。

`G1RedirtyCardsQueue` 就是用来**暂存这些待重新标脏的卡表条目**的队列。

---

### 类层次结构分析

```
G1RedirtyCardsQueueBase  (提供线程本地的 LocalQSet)
        ↑ private 继承
G1RedirtyCardsQueue
        ↑ public 继承
    PtrQueue              (通用指针队列基础设施)
```

关键设计点：

1. **私有继承 `G1RedirtyCardsQueueBase`**：利用 **"base from member"** 惯用法，确保内部的 `LocalQSet`（线程本地队列集合）在 `PtrQueue` 构造之前先完成初始化。

2. **`LocalQSet` 的作用**：作为一个**无竞争的暂存区**，Worker 线程将完成的 Buffer 先放入 `LocalQSet`，最后通过 `flush()` 批量转移到共享的 `G1RedirtyCardsQueueSet`，避免频繁竞争全局锁。

3. **`PtrQueue` 基础设施**：提供通用的 Buffer 管理能力（当前 Buffer 满时触发 `handle_completed_buffer()`，将 Buffer 提交给 `LocalQSet`）。

---

### 工作流程

```
Worker 线程疏散对象
        ↓
发现对象被复制到老年代
        ↓
将卡表条目写入 G1RedirtyCardsQueue（本地 Buffer）
        ↓
Buffer 满 → handle_completed_buffer()
        ↓
Buffer 提交到 LocalQSet（无竞争）
        ↓
flush() → 批量转移到共享 G1RedirtyCardsQueueSet
        ↓
GC 后处理阶段：遍历所有 Buffer，重新标脏卡表
```

---

### 与 `G1RedirtyCardsQueueSet` 的关系

| 类 | 角色 | 线程安全 |
|----|------|---------|
| `G1RedirtyCardsQueue` | **Worker 线程本地队列**，收集待重标脏的卡 | 单线程使用，无竞争 |
| `G1RedirtyCardsQueueSet` | **全局共享队列集合**，汇总所有 Worker 的结果 | 线程安全（有 padding 防伪共享） |

`G1RedirtyCardsQueueSet` 还有两个阶段的设计：
- **收集阶段（collecting）**：各 Worker 通过 `flush()` 向其提交 Buffer
- **处理阶段（processing）**：通过 `take_all_completed_buffers()` 取出所有 Buffer 进行重标脏处理，处理完后自动回到收集阶段

---

### 总结

`G1RedirtyCardsQueue` 本质上是 G1 疏散后**维护跨代引用正确性**的关键组件：它以**线程本地、无竞争**的方式暂存需要重新标脏的卡表条目，通过批量 flush 减少对全局队列的竞争，最终确保 RemSet 在疏散完成后仍然准确反映老年代对年轻代的引用关系。
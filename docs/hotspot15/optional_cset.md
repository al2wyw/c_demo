
## 可选记忆集（Optional Remembered Set）的用途

可选记忆集（Optional RemSet）是 G1 GC 中为**可选回收集（Optional Collection Set）** 服务的一种特殊数据结构，其核心用途是支持 **增量式疏散（Incremental Evacuation）**。

---

### 背景：为什么需要可选记忆集？

G1 的疏散暂停分为两类 Region：

- **初始回收集（Initial Collection Set）**：必须回收的 Region（Young Region + 部分 Old Region）
- **可选回收集（Optional Collection Set）**：可以根据剩余 GC 时间预算**动态决定是否回收**的 Old Region

对于可选回收集中的 Region，G1 不能在 GC 开始时就把它们的 RemSet 全部合并进卡表（因为不确定是否真的会回收它们）。因此需要一种**按需、延迟处理**的机制——这就是可选记忆集的作用。

---

### 可选记忆集的工作机制

从代码中可以看到：

```cpp
void scan_opt_rem_set_roots(HeapRegion* r) {
    // 获取该 Region 对应的可选 RemSet 列表（per-worker 存储）
    G1OopStarChunkedList* opt_rem_set_list = _pss->oops_into_optional_region(r);

    G1ScanCardClosure scan_cl(G1CollectedHeap::heap(), _pss);
    G1ScanRSForOptionalClosure cl(G1CollectedHeap::heap(), &scan_cl);
    // 遍历这些引用，触发对象疏散
    _opt_refs_scanned += opt_rem_set_list->oops_do(&cl, _pss->closures()->strong_oops());
    _opt_refs_memory_used += opt_rem_set_list->used_memory();
}
```

关键点：

1. **Per-Worker 存储**：可选 RemSet 的引用列表是**每个 GC Worker 线程独立持有**的（`_pss->oops_into_optional_region(r)`），因此每个 Worker 都必须扫描自己持有的部分，无需抢占（与强代码根扫描不同）：

    ```cpp
    // The individual references for the optional remembered set are per-worker, so we
    // always need to scan them.
    if (r->has_index_in_opt_cset()) {
        G1EvacPhaseWithTrimTimeTracker timer(...);
        scan_opt_rem_set_roots(r);
    }
    ```

2. **延迟到疏散时处理**：只有当某个可选 Region 真正被加入回收集并开始疏散时，才会调用 `scan_opt_rem_set_roots` 处理其可选 RemSet，而不是在 `merge_heap_roots` 阶段提前合并。

3. **多轮疏散支持**：可选回收集的 Region 可能在**多个疏散轮次**中逐步加入，每轮都通过 `OptScanHR` 阶段扫描新加入 Region 的可选 RemSet，确保跨 Region 引用不被遗漏。

---

### 与普通记忆集的对比

| 特性 | 普通 RemSet | 可选 RemSet |
|------|------------|------------|
| **处理时机** | GC 开始时统一合并进卡表（`merge_heap_roots`） | 疏散时按需扫描（`scan_collection_set_regions`） |
| **存储位置** | 卡表（全局共享） | `G1OopStarChunkedList`（per-worker） |
| **适用 Region** | 初始回收集 Region | 可选回收集 Region |
| **并发控制** | 通过 chunk 抢占分配 | 无需抢占，每个 Worker 扫描自己的部分 |
| **统计指标** | `ScanHRScannedCards` 等 | `ScanHRScannedOptRefs`、`ScanHRUsedMemory` |

---

### 总结

可选记忆集的本质是：**为那些"可能被回收、也可能不被回收"的 Old Region 提供一种轻量级的、按需激活的跨 Region 引用追踪机制**，使 G1 能够在不超出停顿时间目标的前提下，灵活地将更多 Old Region 纳入单次 GC 的回收范围，从而提升老年代的回收效率。
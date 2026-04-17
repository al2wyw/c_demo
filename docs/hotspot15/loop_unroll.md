
## C2 循环展开的代码实现

整个展开过程分为三层：**决策层 → 驱动层 → 执行层**，对应三个核心函数。

---

### 第一层：决策函数 `policy_unroll`（每次判断是否还要再展开一次）

文件：[loopTransform.cpp](/Users/liyang/workspace/jdk15/src/hotspot/share/opto/loopTransform.cpp)（第 755 行）

```cpp
bool IdealLoopTree::policy_unroll(PhaseIdealLoop *phase) {
  CountedLoopNode *cl = _head->as_CountedLoop();

  // 保护：trip count 太小不展开
  if (cl->trip_count() <= (cl->is_normal_loop() ? 2u : 1u)) {
    return false;
  }

  _local_loop_unroll_limit  = LoopUnrollLimit;  // 默认 60（节点数上限）
  _local_loop_unroll_factor = 4;

  // 计算下一轮展开后的 unroll 倍数（每次 ×2）
  int future_unroll_cnt = cl->unrolled_count() * 2;

  // 超过 LoopMaxUnroll（默认 16）就停止
  if (future_unroll_cnt > LoopMaxUnroll) return false;

  // 步长不能太大
  if (abs(cl->stride_con()) > (1<<2)*future_unroll_cnt) return false;

  // 循环体节点数超限就停止
  uint body_size = _body.size();
  if (body_size > (uint)_local_loop_unroll_limit) {
    return false;
  }

  // 通过所有检查 → 允许再展开一次
  return phase->may_require_nodes(estimate);
}
```

**关键点**：`LoopMaxUnroll` 默认值为 **16**，这就是汇编中展开 16 次的来源。每次调用 `policy_unroll` 只决定"再展开一次（×2）"，从 1→2→4→8→16，共调用 4 次。

---

### 第二层：驱动函数（循环调用展开）

文件：[loopTransform.cpp](/Users/liyang/workspace/jdk15/src/hotspot/share/opto/loopTransform.cpp)（第 3319 行）

```cpp
// 在每轮优化 pass 中被调用
bool should_unroll = policy_unroll(phase);   // 问：还要展开吗？

if (should_unroll && !should_peel) {
    phase->do_unroll(this, old_new, true);   // 答：要，执行一次展开（×2）
}
```

**这个函数在每轮 `optimize_loop` 中被反复调用**，每次只展开一倍，直到 `policy_unroll` 返回 false 为止：

```
第1轮：unrolled_count=1 → future=2  ≤ 16 → do_unroll → 循环体复制1份
第2轮：unrolled_count=2 → future=4  ≤ 16 → do_unroll → 循环体复制1份
第3轮：unrolled_count=4 → future=8  ≤ 16 → do_unroll → 循环体复制1份
第4轮：unrolled_count=8 → future=16 ≤ 16 → do_unroll → 循环体复制1份
第5轮：unrolled_count=16→ future=32 > 16 → 返回 false，停止
```

---

### 第三层：执行函数 `do_unroll`（每次把循环体复制一份）

文件：[loopTransform.cpp](/Users/liyang/workspace/jdk15/src/hotspot/share/opto/loopTransform.cpp)（第 1841 行）

```cpp
void PhaseIdealLoop::do_unroll(IdealLoopTree *loop, Node_List &old_new,
                                bool adjust_min_trip) {
  // Step 1: 调整 limit，让主循环只跑偶数次
  //   new_limit = limit - stride
  //   这样 pre-loop 处理剩余的奇数次迭代
  new_limit = _igvn.intcon(limit->get_int() - stride_con);

  // Step 2: 更新 trip_count 和 unrolled_count
  loop_head->set_trip_count(old_trip_count / 2);   // trip 减半
  loop_head->double_unrolled_count();              // 记录已展开倍数 ×2

  // ★ Step 3: 核心！调用 clone_loop 复制整个循环体
  clone_loop(loop, old_new, dd, IgnoreStripMined);

  // Step 4: 把克隆体的回边接到原循环体的入口
  //   形成：[clone body] → [original body] → back edge
  for (DUIterator_Fast jmax, j = loop_head->fast_outs(jmax); j < jmax; j++) {
    Node* phi = loop_head->fast_out(j);
    if (phi->is_Phi() && phi->in(0) == loop_head) {
      Node *newphi = old_new[phi->_idx];
      // 原循环的入口 ← 克隆体的出口
      phi->set_req(LoopNode::EntryControl,
                   newphi->in(LoopNode::LoopBackControl));
      // 克隆体的回边 ← 原循环的回边
      newphi->set_req(LoopNode::LoopBackControl,
                      phi->in(LoopNode::LoopBackControl));
      // 原循环的回边 ← 死节点（kill backedge）
      phi->set_req(LoopNode::LoopBackControl, C->top());
    }
  }
}
```

---

### `clone_loop` 的节点复制实现

文件：[loopopts.cpp](/Users/liyang/workspace/jdk15/src/hotspot/share/opto/loopopts.cpp)（第 2041 行）

```cpp
void PhaseIdealLoop::clone_loop(IdealLoopTree *loop, Node_List &old_new, ...) {

  // ★ Step 1: 遍历循环体中的每个节点，逐一 clone
  for (uint i = 0; i < loop->_body.size(); i++) {
    Node *old = loop->_body.at(i);
    Node *nnn = old->clone();              // 直接复制节点（包括操作码、类型）
    old_new.map(old->_idx, nnn);           // 建立 旧节点→新节点 的映射表
    _igvn.register_new_node_with_optimizer(nnn);
  }

  // ★ Step 2: 修复新节点的边
  //   - 如果输入节点在循环外 → 保持原引用（loop-invariant）
  //   - 如果输入节点在循环内 → 替换为对应的克隆节点
  for (uint i = 0; i < loop->_body.size(); i++) {
    Node *old = loop->_body.at(i);
    Node *nnn = old_new[old->_idx];
    for (uint j = 0; j < nnn->req(); j++) {
      Node *n = nnn->in(j);
      if (n) {
        IdealLoopTree *old_in_loop = get_loop(has_ctrl(n) ? get_ctrl(n) : n);
        if (loop->is_member(old_in_loop))
          nnn->set_req(j, old_new[n->_idx]);  // 内部引用 → 指向克隆节点
      }
    }
  }
}
```

---

### 整体流程图

```
optimize_loop() 被反复调用（每轮优化 pass）
        │
        ▼
policy_unroll()  ──── false ──→ 停止展开
        │ true
        ▼
do_unroll()
  ├─ 调整 limit（new_limit = limit - stride）
  ├─ double_unrolled_count()（记录 ×2）
  ├─ clone_loop()  ← ★ 核心：逐节点 clone
  │     ├─ for each node: node->clone()
  │     └─ 修复内部边引用
  └─ 重接回边（clone body → original body）

第1次：1个循环体 → 2个循环体（unrolled=2）
第2次：2个循环体 → 4个循环体（unrolled=4）
第3次：4个循环体 → 8个循环体（unrolled=8）
第4次：8个循环体 → 16个循环体（unrolled=16）
                              ↓
                    IGVN 全局优化（公共子表达式消除等）
                              ↓
                    生成汇编：16 次 add %ecx,%edx
```

---

### 为什么说是"基于模板"

`clone_loop` 的本质就是 **`node->clone()`**——把循环体中的每个 IR 节点（`AddINode`、`StoreINode` 等）**原样复制一份**，然后修复引用关系。C2 **不会**在复制之后对展开后的多份节点做代数化简（如把 16 个 `AddI` 合并为 `ShlI + AddI`）。

这就是"基于模板"的含义：**复制节点，不做跨副本的代数优化**。后续的 IGVN（Ideal Graph Value Numbering）虽然会做一些化简，但对这种带有 RAW 依赖链的累加模式无能为力。

### 注意
C2没有做展开后的强度削减（Strength Reduction）
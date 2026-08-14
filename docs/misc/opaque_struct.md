# `_jmethodID` 是"不完整类型"（incomplete/opaque struct），不是"空 struct"

```c
struct _jmethodID;
typedef struct _jmethodID *jmethodID;
```

### 不完整类型的关键性质

对不完整类型 `T`：

| 操作 | 允许？ |
|---|---|
| 声明 `T*` 指针 | ✅ 允许 |
| `sizeof(T)` | ❌ 编译错误 |
| 解引用 `T*`、访问成员 | ❌ 编译错误 |
| 声明变量 `T x;` | ❌ 编译错误 |

正是这些"限制"，**JNI 头文件的使用者只能拿着 `jmethodID` 传来传去，不能窥探它内部结构**。

### 1. 类型安全（vs `void*`）

如果 JNI 直接用 `void*` 表示 method ID，那么 `jmethodID`、`jfieldID`、`jclass` 就可以随意互相赋值，编译器一点错都不报。

这就是**"tag types" 惯用法**（有时也叫 "opaque handle" 或 "phantom type"）。

### 2. 信息隐藏 / ABI 稳定

不完整类型让 JNI 使用者**无法依赖内部布局**。HotSpot 可以任意重构 `Method` 类的成员，JNI 用户代码依然照常编译、照常运行——只要指针语义不变即可。

### 3、C 侧和 C++ 侧的差异

- **C 分支**：
  ```c
  struct _jobject;
  struct _jclass;
  struct _jmethodID;
  ...
  typedef struct _jobject   *jobject;
  typedef struct _jclass    *jclass;
  typedef struct _jmethodID *jmethodID;
  ```
  全部走"不完整类型 + typedef 成指针"的路子。

- **C++ 分支**（有时用另一套）：
  ```cpp
  class _jobject {};
  class _jclass : public _jobject {};
  class _jmethodID {};
  ...
  typedef _jobject   *jobject;
  typedef _jclass    *jclass;
  typedef _jmethodID *jmethodID;
  ```
  在 C++ 里为了能利用继承做隐式向上转换（`jclass` → `jobject`），会给出**真正的空 class 定义**。这时它才是你说的"空 struct/class"。

### 4、三者的能力对比总表

| 操作 | (A) 不完整类型 `struct Foo;` | (B) 空 struct（C） | (B) 空 struct（C++） | (C) 空 class（C++） |
|---|---|---|---|---|
| 声明指针 `Foo*` | ✅ | ✅ | ✅ | ✅ |
| 声明变量 `Foo x;` | ❌ 编译错误 | ⚠️ 标准 C 非法，GCC 扩展允许 | ✅ | ✅ |
| `sizeof(Foo)` | ❌ 编译错误 | ⚠️ GCC 扩展下为 0 | **≥ 1**（通常 1） | **≥ 1**（通常 1） |
| 解引用 `Foo*` | ❌ 编译错误 | ✅ | ✅ | ✅ |
| 访问成员 | ❌ 无成员可访问 | ❌ 无成员 | ❌ 无成员 | ❌ 无成员 |
| 用作数组元素 | ❌ | ⚠️ GCC 扩展 | ✅ | ✅ |
| 用作函数返回值/参数（按值） | ❌ | ⚠️ GCC 扩展 | ✅ | ✅ |
| 继承 | ❌ | — | — | ✅ |
| 存在成员函数、构造/析构 | ❌ | ❌（C 无此概念） | ✅ | ✅ |
| 空基类优化（EBO） | — | — | — | ✅ 常见 |

___

# `struct jvmtiEnv` 与 `class JvmtiEnv` 的关系

> **`struct jvmtiEnv` 是暴露给 agent 的 C ABI 接口（函数表指针的"外壳"），`class JvmtiEnv` 是 HotSpot 内部的完整 C++ 实现。二者通过 `JvmtiEnvBase` 里内嵌的成员 `_jvmti_external` 拼在同一个对象里(POD 兼容布局)，同一份内存双向可换算。**

## 总结对照表

| 维度 | `struct jvmtiEnv` (C 侧)                  | `class JvmtiEnv` (C++ 侧)                                  |
|------|------------------------------------------|-----------------------------------------------------------|
| 定义位置 | `jvmti.h`（规范）                            | 由 `jvmtiHpp.xsl` 生成的 `jvmtiEnv.hpp`                       |
| 类型语义 | C ABI 结构体，只是函数表指针，让任何语言的 agent 都能用 | C++ 类，继承自 `JvmtiEnvBase`，需要 class、继承、成员函数等特性              |
| 数据成员 | 只有 `functions`（一个指针）                     | 无自身字段，业务字段全在 `JvmtiEnvBase`                               |
| 谁看到 | JVMTI Agent（外部）                          | HotSpot 内部代码                                              |
| 谁创建 | 不能独立创建                                   | `JvmtiEnv::create_a_jvmti(version)` 用 new 分配              |
| 在内存中的位置 | 是 `JvmtiEnvBase::_jvmti_external` 成员     | 完整对象；`_jvmti_external` 只是它的一个成员                           |
| 相互换算 | `(jvmtiEnv*)&jvmti_env->_jvmti_external` | `JvmtiEnvBase::JvmtiEnv_from_jvmti_env(env)`（`env` 减去偏移，container_of 换算） |
| 类比 | `JNIEnv`（同款设计）                           | `JavaThread`（同款设计）                                        |

**一句话再收束**：`struct jvmtiEnv` 是 `class JvmtiEnv` 挂在自己身上的**"C ABI 名片"**，agent 通过这张名片调用进来，HotSpot 立刻通过 `container_of` 式的偏移换算找回真正的 `class JvmtiEnv` 对象，然后走完整的 C++ 成员函数分派。二者是**同一对象的两个视图**，不是两个独立类型。

___

**把struct或class放在class的首位(POD标准布局)，可以利用内存地址(对象基址)重叠，此时指针可以互相转换**
```cpp
typedef struct A {
    int i;
} strA;
typedef struct B {
    strA a;
    int j;
} strB;
strB* b;
strA* a = (strA*) b;
```


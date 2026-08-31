## Graal编译器加载

**JVMCI 加载 Graal**：HotSpot 在启动后期通过 `JVMCI::initialize_compiler` → `JVMCIRuntime::call_getCompiler`,借助 `JavaCalls`（传统模式）或 JNI（libgraal 模式）调用 Java 类 `HotSpotJVMCIRuntime.runtime() / getCompiler()`, 后者用 `ServiceLoader` 找到 `HotSpotGraalCompilerFactory` 并实例化 Graal。

**Graal 自身怎么变成机器码**：
- **传统模式**：作为普通 Java 类,由 HotSpot 的解释器 + **C1** 编译（在 [jvmciCompiler.cpp](/jdk15/src/hotspot/share/jvmci/jvmciCompiler.cpp) 的 `force_comp_at_level_simple` 强制不让 Graal 编译自己）。
- **libgraal 模式**：**在 JDK 构建阶段就用 Substrate VM Native Image 预先 AOT 编译**成 `libjvmcicompiler.so`,并连带一个精简的 GC/线程运行时，运行时通过 `os::dll_load` + `JNI_CreateJavaVM` 创建一个SVM实例并加载到 HotSpot 进程里, SVM和 HotSpot 通过 JNI 通信。


### 两种模式的关键差异

| 维度 | UseJVMCINativeLibrary=false（传统） | UseJVMCINativeLibrary=true（libgraal） |
|---|---|---|
| Graal 存放位置 | 与用户代码同在 HotSpot heap | 独立的 SVM heap（libjvmcicompiler） |
| Graal 编译成机器码方式 | 启动时解释,由 C1 逐步 JIT | 构建 JDK 时 Native Image **AOT 预编译** |
| 启动时间 | 慢（Graal 自身要预热） | 快（一开始就是本地代码） |
| 编译对用户堆的影响 | Graal 的临时对象污染用户 GC | 完全隔离,不影响用户 GC |
| `_compiler_runtime` / `_java_runtime` | 同一个 | 两个独立的 |


### 调用链路 
```text
JVMCI::initialize_compiler(TRAPS)                          [jvmci.cpp:53]
  └── JVMCI::compiler_runtime()->call_getCompiler(CHECK)   [jvmciRuntime.cpp:617]
        │
        ├── THREAD_JVMCIENV(JavaThread::current())         ← 关键:构造 JVMCIEnv 对象
        │     └── JVMCIEnv::JVMCIEnv(...) 构造函数
        │           └── init_env_mode_runtime(thread, NULL) [jvmciEnv.cpp:196]
        │                 │
        │                 └── init_shared_library(thread)   [jvmciEnv.cpp:142]  ★libgraal 加载点
        │                         ├── os::dll_load("libjvmcicompiler.so")
        │                         ├── os::dll_lookup("JNI_CreateJavaVM")
        │                         └── JNI_CreateJavaVM(&the_javavm, &env, ...)
        │                                    ↓
        │                       _shared_library_javavm = the_javavm
        │                       _env = env  (SVM 侧的 JNIEnv*)
        │
        ├── get_HotSpotJVMCIRuntime(JVMCI_CHECK)
        │     └── initialize_HotSpotJVMCIRuntime            [jvmciRuntime.cpp:709]
        │           └── JVMCIENV->call_HotSpotJVMCIRuntime_runtime()   ★通过 (SVM的)JNI 跨vm反向调用 or JavaCalls 调用本地Java类，由_is_hotspot进行区分
        │                 └── jni()->CallStaticObjectMethod(
        │                         JNIJVMCI::HotSpotJVMCIRuntime::clazz(),
        │                         JNIJVMCI::HotSpotJVMCIRuntime::runtime_method())
        │                 └── JavaCalls::call_static(&result,
        │                           HotSpotJVMCI::HotSpotJVMCIRuntime::klass(),
        │                           vmSymbols::runtime_name(),
        │                           vmSymbols::runtime_signature(), &jargs, ...);
        └── JVMCIENV->call_HotSpotJVMCIRuntime_getCompiler(jvmciRuntime, ...)
              └── jni()->CallObjectMethod(runtime.as_jobject(),
                       JNIJVMCI::HotSpotJVMCIRuntime::getCompiler_method())
              └── JavaCalls::call_virtual(&result, HotSpotJVMCI::HotSpotJVMCIRuntime::klass(),
                            vmSymbols::getCompiler_name(),
                            vmSymbols::getCompiler_signature(), &jargs, ...); 
```
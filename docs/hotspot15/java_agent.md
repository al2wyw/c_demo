# `DEF_Agent_OnLoad` 触发路径与 `JPLISAgent` 存储位置深度剖析

### 完整调用链

```
JNI_CreateJavaVM (jni.cpp)
   └─ Threads::create_vm (thread.cpp)
        ├─ Arguments::parse (处理 -javaagent 参数)
        │     └─ 把 instrument agent 加进 Arguments::agents() 链表
        ├─ Threads::convert_vm_init_libraries_to_agents (处理 -Xrun 兼容)
        └─ Threads::create_vm_init_agents // Create agents for -agentlib:  -agentpath:  and converted -Xrun
              ├─ JvmtiExport::enter_onload_phase()
              ├─ for each agent:
              │    ├─ lookup_agent_on_load  // dlopen libinstrument.so -> dlsym "Agent_OnLoad"
              │    └─ (*on_load_entry)(&main_vm, options, NULL)  ★调用点★
              │             │
              │             ▼
              │      DEF_Agent_OnLoad @ InvocationAdapter.c:146
              │      (= Agent_OnLoad in libinstrument.so)
              │             │
              │             ├─ createNewJPLISAgent(vm, &agent)                                                                            
              │             │     ├─ vm->GetEnv(&jvmtiEnv, JVMTI_VERSION_1_1)   // 新建 jvmtiEnv
              │             │     ├─ allocateJPLISAgent(jvmtiEnv)                           
              │             │     │    └─ jvmtiEnv->Allocate(sizeof(JPLISAgent))   ◄── 内存来源 
              │             │     └─ initializeJPLISAgent(agent, vm, jvmtiEnv)              
              │             │          ├─ agent->mJVM = vm                                  
              │             │          ├─ agent->mNormalEnvironment.mJVMTIEnv = jvmtiEnv    
              │             │          ├─ agent->mNormalEnvironment.mAgent    = agent       
              │             │          ├─ jvmtiEnv->SetEnvironmentLocalStorage(jvmtiEnv, &agent->mNormalEnvironment)  ◄── 存放位置                           
              │             │          └─ callbacks.VMInit = &eventHandlerVMInit // 注册 VMInit 回调
              │             ├─ 解析 jar 的 manifest（Premain-Class / Boot-Class-Path）
              │             ├─ convertCapabilityAttributes(...)
              │             └─ recordCommandLineData(agent, premainClass, options)
              │
              └─ JvmtiExport::enter_primordial_phase()
```

### 顺带说下另外两个入口

同一个文件里还有两个 JVMTI agent 入口点，它们的触发路径**不同**：

- `DEF_Agent_OnAttach`（[InvocationAdapter.c:313](/jdk15/src/java.instrument/share/native/libinstrument/InvocationAdapter.c)）—— 通过 **Attach 机制**（`VirtualMachine.attach(pid).loadAgent(...)`）触发；由 attach listener 线程调用。(通过unix domain socket向目标vm传递load命令和对应的agent路径)
- `loadAgent`（[InvocationAdapter.c:453](/jdk15/src/java.instrument/share/native/libinstrument/InvocationAdapter.c)）—— 用于 **Launcher-Agent-Class**（把 agent 打进主 JAR 的 manifest 里）时，由 Java launcher 触发。

三条路径都会调用 `createNewJPLISAgent`，都会分配一个 `JPLISAgent` 结构。

### 多 `-javaagent` 情况

如果命令行有多个 `-javaagent:` 参数:

- **每个 `-javaagent` 都会导致 JVM 调用一次 `Agent_OnLoad`**；
- **每次调用都会 `createNewJPLISAgent` 一次** —— 产生一个新的 `JPLISAgent` + 一个新的 `jvmtiEnv`；
- 每个 agent 各自把自己的 `JPLISEnvironment*` 塞进**自己那份** `jvmtiEnv` 的 Local Storage。

也就是说，**`JPLISAgent` 的实例数 = `-javaagent` 参数个数 = jvmtiEnv 个数**，彼此隔离。每个 agent 各自拥有独立的 `jvmtiEnv` 和 `JPLISAgent`，通过"env local storage"实现天然隔离与 O(1) 反查，既不用全局单例，也不用 map 查找。这是 JVMTI **"agent-per-environment"** 架构的一次非常典型的应用。
//
// Created by 李扬 on 2026/5/31.
//
//（main 当裁判版）：双 flag spin-wait 严格乒乓同步
// main 负责鸣枪 + 等收尾 + 重置状态，A/B 完全对称
//
// __atomic 是 GCC 内置函数‌，需 GCC 4.7+，C11 标准原子操作是 <stdatomic.h> 中的 atomic_* 函数
// 在单线程下cpu保证内存最终语义一致，在多线程下需要额外的内存序来实现跨线程的内存顺序控制
// 当一个线程以release语义写入原子变量，另一个线程以acquire语义读取同一变量时，前者的所有前序内存操作对后者可见(happen-before)
//内存序	    原子性	同步性	性能开销
//relaxed	✓	    ✗	低
//acquire/release	✓	✓	中
//seq_cst	✓	✓	高
// 无锁设计基于 原子操作 和 内存序控制
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int LOOP_COUNT = 10000;
#define NUM 2

volatile int a = 0;
volatile int b = 0;

// 同步用的标志（必须用原子操作访问）
volatile int round_no = -1;   // 当前轮次，-1 表示尚未起跑
volatile int a_done   = 0;    // A 本轮是否完成核心逻辑
volatile int b_done   = 0;    // B 本轮是否完成核心逻辑

volatile int taken = 0;
volatile int untaken = 0;

void* workerA(void* arg) {
    int count = 0;
    int fail  = 0;
    for (int i = 0; i < LOOP_COUNT; i++) {
        // ① 等待 main 鸣枪：round_no 推进到 i
        while (__atomic_load_n(&round_no, __ATOMIC_ACQUIRE) != i) ;

        // ② 核心逻辑：典型 Store-Load 模式
        a = 1;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        //__atomic_thread_fence(__ATOMIC_RELEASE);
        //__atomic_thread_fence(__ATOMIC_ACQUIRE);
        //__atomic_thread_fence(__ATOMIC_ACQ_REL);
        if (b == 0) {
            count++;
            __atomic_fetch_add(&taken, 1, __ATOMIC_SEQ_CST);
        } else {
            fail++;
            __atomic_fetch_add(&untaken, 1, __ATOMIC_SEQ_CST);
        }

        // ③ 举手示意本轮已完成（裁判会看到）
        __atomic_store_n(&a_done, 1, __ATOMIC_RELEASE);
    }
    printf("workerA finished: %d, fail: %d\n", count, fail);
    return NULL;
}

void* workerB(void* arg) {
    int count = 0;
    int fail  = 0;
    for (int i = 0; i < LOOP_COUNT; i++) {
        // ① 等待 main 鸣枪
        while (__atomic_load_n(&round_no, __ATOMIC_ACQUIRE) != i) ;

        // ② 核心逻辑
        b = 1;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (a == 0) {
            count++;
            __atomic_fetch_add(&taken, 1, __ATOMIC_SEQ_CST);
        } else {
            fail++;
            __atomic_fetch_add(&untaken, 1, __ATOMIC_SEQ_CST);
        }

        // ③ 举手示意本轮已完成
        __atomic_store_n(&b_done, 1, __ATOMIC_RELEASE);
    }
    printf("workerB finished: %d, fail: %d\n", count, fail);
    return NULL;
}

int main(int argc, char *argv[]) {

    int loop = (argc > 1) ? atoi(argv[1]) : LOOP_COUNT;
    LOOP_COUNT = loop;

    pthread_t threads[NUM];
    for (int i = 0; i < NUM; i++) {
        pthread_create(&threads[i], NULL, i % 2 == 0 ? workerA : workerB, NULL);
    }
    sleep(1);  // 等两个 worker 都阻塞在 ① 处
    int taken_count = 0;
    int untaken_count = 0;

    // main 当裁判：每轮鸣枪 → 等双方收尾 → 重置 → 下一轮
    for (int i = 0; i < LOOP_COUNT; i++) {
        // 重置本轮的实验变量（必须在鸣枪之前完成）
        taken = 0;
        untaken = 0;

        a = 0;
        b = 0;

        a_done = 0;
        b_done = 0;

        // 鸣枪：让 A、B 同时通过 ①
        __atomic_store_n(&round_no, i, __ATOMIC_RELEASE);

        // 等双方都举手
        while (__atomic_load_n(&a_done, __ATOMIC_ACQUIRE) == 0) ;
        while (__atomic_load_n(&b_done, __ATOMIC_ACQUIRE) == 0) ;

        //判断是否命中
        if (taken == 2) {
            taken_count++;
        }
        if (untaken == 2) {
            untaken_count++;
        }
    }
    printf("taken: %d, untaken: %d\n", taken_count, untaken_count);

    for (int i = 0; i < NUM; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

/*
untaken=0的原因:
A、B 的"赢跑"严重不均（98703 vs 1297），让两者的 Wa/Wb 几乎不可能"同时完成于双方读之前"
解决方法:
显著拉大循环数到上亿次，并把两个线程绑到不同物理核心
 */
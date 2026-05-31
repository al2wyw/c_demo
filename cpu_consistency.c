//
// Created by 李扬 on 2026/5/31.
//
// failed !!!
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int LOOP_COUNT = 10000;
int NUM = 2;
volatile int a = 0;
volatile int b = 0;
volatile int flag = 0;

void* workerA(void* arg) {

    int count = 0;
    int fail = 0;
    for (int i = 0; i < LOOP_COUNT; i++) {
        while (flag % 2 == 0); //怎么保证每次循环两个线程都可以同时执行，无论谁先执行完都会等对方
        a = 0;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        a = 1;
        //插入mfence
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        //插入release语义
        //__atomic_thread_fence(__ATOMIC_RELEASE);
        //插入acquire语义
        //__atomic_thread_fence(__ATOMIC_ACQUIRE);
        //插入acquire release语义
        //__atomic_thread_fence(__ATOMIC_ACQ_REL);
        if (b == 0) {
            count++;
        } else {
            fail++;
        }
        //__atomic_fetch_add(&flag, 1, __ATOMIC_SEQ_CST);
    }
    printf("workerA finished: %d, fail: %d\n", count, fail);
    return NULL;
}

void* workerB(void* arg) {
    int count = 0;
    int fail = 0;
    for (int i = 0; i < LOOP_COUNT; i++) {
        while (flag % 2 == 0);
        b = 0;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        b = 1;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (a == 0) {
            count++;
        } else {
            fail++;
        }
    }
    printf("workerB finished: %d, fail: %d\n", count, fail);
    return NULL;
}

void main(int argc, char *argv[]) {

    int loop = (argc > 1) ? atoi(argv[1]) : LOOP_COUNT;
    LOOP_COUNT = loop;

    pthread_t threads[NUM];
    for (int i = 0; i < NUM; i++) {
        pthread_create(&threads[i], NULL, i % 2 == 0 ? workerA: workerB, NULL);
    }
    sleep(1);
    flag = 1;
    for (int i = 0; i < NUM; i++) {
        pthread_join(threads[i], NULL);
    }
}

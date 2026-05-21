//
// Created by 李扬 on 2026/5/19.
//
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#ifdef __linux__
#include <sched.h>
#endif

volatile int data_ready = 0;
typedef struct {
    //int arr_padding1[16];
    volatile int produce_data;
    //int arr_padding2[16];
    volatile int consume_data;
} data_t;
data_t shared_data;
int LOOP_COUNT = 1000000;

// 把当前线程绑定到指定 CPU 上（仅在 Linux 上生效）
static void bind_to_cpu(pthread_t th, int cpu_id, const char *thread_name) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    int rc = pthread_setaffinity_np(th, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        fprintf(stderr, "[%s] pthread_setaffinity_np failed: %s\n",
                thread_name, strerror(rc));
    } else {
        printf("[%s] bound to CPU %d\n", thread_name, cpu_id);
    }
#else
    (void)cpu_id;
    printf("[%s] cpu affinity not supported on this platform\n", thread_name);
#endif
}

// 线程参数：把要绑的 cpu 编号传进去，让线程自己第一时间绑核
typedef struct {
    int cpu_id;
    const char *name;
} thread_arg_t;

// 生产者线程函数
void* producer(void* arg) {
    thread_arg_t *ta = (thread_arg_t*)arg;
    bind_to_cpu(pthread_self(), ta->cpu_id, ta->name);

    for (int i = 0; i < LOOP_COUNT; i++) {

        // 生产数据
        shared_data.produce_data = i;
        while (shared_data.consume_data != i) {
            // busy wait
        }

    }
    printf("producer finished\n");
    return NULL;
}

// 消费者线程函数
void* consumer(void* arg) {
    thread_arg_t *ta = (thread_arg_t*)arg;
    bind_to_cpu(pthread_self(), ta->cpu_id, ta->name);

    for (int i = 0; i < LOOP_COUNT; i++) {

        // 消费数据
        shared_data.consume_data = i;
        while (shared_data.produce_data != i) {
            // busy wait
        }

    }
    printf("consumer finished\n");
    return NULL;
}

//两个线程通过mutex的条件变量进行通信
void main(int argc, char *argv[]) {
    pthread_t producer_thread, consumer_thread;

    int loop = (argc > 1) ? atoi(argv[1]) : LOOP_COUNT;
    LOOP_COUNT = loop;

    // 不同的线程设置不同的 cpu affinity
    // 让线程自己在入口处第一时间绑核，避免主线程晚绑导致的初始迁移
    thread_arg_t p_arg = { .cpu_id = 1, .name = "producer" };
    thread_arg_t c_arg = { .cpu_id = 2, .name = "consumer" };
    pthread_create(&producer_thread, NULL, producer, &p_arg);
    pthread_create(&consumer_thread, NULL, consumer, &c_arg);

    printf("start\n");
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
}
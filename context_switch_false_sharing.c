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


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
volatile int data_ready = 0;
typedef struct {
    //int arr_padding1[16];
    int produce_data;
    //int arr_padding2[16];
    int consume_data;
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

// 生产者线程函数
void* producer(void* arg) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        // 生产数据
        while (data_ready) {
            pthread_cond_wait(&cond, &mutex);
        }
        shared_data.produce_data = i;
        data_ready = 1;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    printf("producer finished\n");
    return NULL;
}

// 消费者线程函数
void* consumer(void* arg) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        // 等待数据就绪
        while (!data_ready) {
            pthread_cond_wait(&cond, &mutex);
        }

        shared_data.consume_data = i;
        data_ready = 0;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    printf("consumer finished\n");
    return NULL;
}

//两个线程通过mutex的条件变量进行通信
void main(int argc, char *argv[]) {
    pthread_t producer_thread, consumer_thread;

    int loop = (argc > 1) ? atoi(argv[1]) : LOOP_COUNT;
    LOOP_COUNT = loop;

    //不同的线程设置不同的 cpu affinity
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    bind_to_cpu(producer_thread, 0, "producer");
    bind_to_cpu(consumer_thread, 1, "consumer");

    printf("start\n");
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
}
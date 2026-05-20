//
// Created by 李扬 on 2026/5/19.
//
// taskset -c 0 cpu-migrations大幅减少，但是context switch不变，主要的延迟都在cache-miss上
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
volatile int data_ready = 0;
int shared_data = 0;
int LOOP_COUNT = 1000000;

// 生产者线程函数
void* producer(void* arg) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        // 生产数据
        while (data_ready) {
            pthread_cond_wait(&cond, &mutex);
        }
        shared_data = i;
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

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    printf("start\n");
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
}
//
// Created by 李扬 on 2026/5/19.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    //int arr_padding1[32];
    volatile long produce_data;
    //int arr_padding2[32];
    volatile long consume_data;
} data_t;
data_t shared_data;
long LOOP_COUNT = 200000000;   // 2 亿次，让差异充分体现

// 生产者线程函数
void* producer(void* arg) {
    for (long i = 0; i < LOOP_COUNT; i++) {

        // 生产数据
        shared_data.produce_data += i;

    }
    printf("producer finished\n");
    return NULL;
}

// 消费者线程函数
void* consumer(void* arg) {

    for (long i = 0; i < LOOP_COUNT; i++) {

        // 消费数据
        shared_data.consume_data += i;

    }
    printf("consumer finished\n");
    return NULL;
}

//两个线程并行的跑
void main(int argc, char *argv[]) {
    pthread_t producer_thread, consumer_thread;

    if (argc > 1) LOOP_COUNT = atol(argv[1]);

    printf("LOOP_COUNT=%ld, sizeof(data_t)=%zu, "
           "&produce_data=%p, &consume_data=%p, distance=%ld bytes\n",
           LOOP_COUNT, sizeof(data_t),
           (void*)&shared_data.produce_data,
           (void*)&shared_data.consume_data,
           (char*)&shared_data.consume_data - (char*)&shared_data.produce_data);

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    printf("start\n");
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    printf("done. produce_data=%ld, consume_data=%ld\n",
           shared_data.produce_data, shared_data.consume_data);
}
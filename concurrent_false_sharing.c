//
// Created by 李扬 on 2026/5/19.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

int NUM = 10;
typedef struct {
    //int arr_padding1[32];
    volatile long value;
} data_t;
data_t *shared_data;
long LOOP_COUNT = 200000000;   // 2 亿次，让差异充分体现

void* worker(void* arg) {
    int index = *(int*)arg;
    for (long i = 0; i < LOOP_COUNT; i++) {
        shared_data[index].value += i;
    }
    printf("worker %d finished\n", index);
    return NULL;
}

//两个线程并行的跑
void main(int argc, char *argv[]) {

    if (argc > 1) LOOP_COUNT = atol(argv[1]);
    if (argc > 2) NUM = atoi(argv[2]);

    shared_data = malloc(sizeof(data_t) * NUM);
    if (shared_data == NULL) {
        perror("malloc");
        exit(1);
    }

    printf("LOOP_COUNT=%ld, sizeof(data_t)=%zu, "
           "&shared_data[0]=%p, &shared_data[1]=%p, distance=%ld bytes\n",
           LOOP_COUNT, sizeof(data_t),
           (void*)&shared_data[0],
           (void*)&shared_data[1],
           (char*)&shared_data[1] - (char*)&shared_data[0]);


    pthread_t threads[NUM];
    int index[NUM];
    for (int i = 0; i < NUM; i++) {
        index[i] = i;
        pthread_create(&threads[i], NULL, worker, &index[i]);
    }

    printf("start\n");
    for (int i = 0; i < NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < NUM; i++) {
        printf("done. value=%ld\n", shared_data[i].value);
    }

    free(shared_data);
}
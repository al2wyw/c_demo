//
// Created by root on 6/4/23.
//

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

#define THREAD_SIZE     10

int inc(int *value, int add) {
    int old;
    __asm__ volatile (
    "lock; xaddl %2, %1;"
    : "=a" (old)
    : "m" (*value), "a" (add)
    : "cc", "memory"
    );
    return old;
}

//callback
void *func(void *arg) {
    int *pcount = (int *) arg;
    int i = 0;
    while (i++ < 10000) {
        inc(pcount, 1);

        usleep(1);
    }
}


int main() {
    pthread_t th_id[THREAD_SIZE] = {0};

    int i = 0;
    int count = 0;

    for (i = 0; i < THREAD_SIZE; i++) {
        int ret = pthread_create(&th_id[i], NULL, func, &count);
        if (ret) {
            break;
        }
    }
    for (i = 0; i < 100; i++) {
        printf("count --> %d\n", count);

        sleep(2);
    }
}
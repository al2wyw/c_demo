//
// Created by 李扬 on 2023/5/30.
//
#include<stdio.h>
#include<pthread.h>
#include<errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int val = 0;

void *run() {
    sleep(1);
    printf("%d get value\n", val);
    val++;
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    printf("%ld set value to 1\n", time.tv_nsec);

    return 0;
}

int main() {
    pthread_t thrd1;
    if (pthread_create(&thrd1, NULL, run, NULL) != 0)
    {
        printf("thread error:%s \n", strerror(errno));
        return 1;
    }
    pthread_detach(thrd1);

    while (!val);
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    printf("%ld get value\n", time.tv_nsec);

    sleep(1);
}

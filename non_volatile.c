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
// val++; no O 都是一样的
// movl    16948(%rip), %eax       ## <_val>
// addl    $1, %eax
// movl    %eax, 16939(%rip)       ## <_val>
// val++; O1 都是一样的
// addl    $1, 16915(%rip)         ## <_val>

// while (!val); no O
// int val = 0:
// cmpl    $0, 16792(%rip)      ##  <_val>
// volatile int val = 0:
// movl    16793(%rip), %eax    ##  <_val>
// cmpl    $0, %eax
// while (!val);  O1
// int val = 0:
// 被优化掉了
// volatile int val = 0:
// cmpl    $0, 16793(%rip)      ##  <_val>
// 直接比较imm和内存，从内存读取到reg再比较imm和reg，有什么不同 ???

void *run() {
    sleep(1);
    printf("%d first get value\n", val);
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

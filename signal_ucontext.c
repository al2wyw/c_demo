//
// Created by 李扬 on 2023/4/29.
//
// REG_RIP、REG_RSP 等枚举是 GNU 扩展
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ucontext.h>
#include <stdint.h>

void get_format_time_ms(char *str_time);

typedef void (*SigHandler)(int);
typedef void (*SigAction)(int, siginfo_t*, void*);

/**
| 平台 | 架构 | 展开 |
|------|------|------|
| macOS | x86_64 | `uc_mcontext->__ss.__rip` |
| macOS | arm64  | `uc_mcontext->__ss.__pc` |
| Linux | x86_64 | `uc_mcontext.gregs[REG_RIP]` |
| Linux | i386 | `uc_mcontext.gregs[REG_EIP]` |
| Linux | arm64 | `uc_mcontext.pc`（不是 gregs 数组） |
 */
#ifdef __APPLE__
#  define REG(l, m)  _ucontext->uc_mcontext->__ss.__##m
#else
#  define REG(l, m)  _ucontext->uc_mcontext.gregs[REG_##l]
#endif

long getThreadId() {
    pthread_t cur = pthread_self();
    return (long)cur;
}

void* run(void* arg)
{
    while (1) {
        printf("start to trigger SIGSEGV in few seconds %lu\n", getThreadId());
        sleep(3);
        int a[3] = {0};
        fprintf(stdout, "a[3] = %d\n", a[-11111111111111]);//trigger SIGSEGV
    }
}

void signalHandler(int signo, siginfo_t* siginfo, void* ucontext) {
    char timeStamp[32];
    get_format_time_ms(timeStamp);
    fprintf(stdout, "%s caught %s signal %lu\n", timeStamp, strsignal(signo), getThreadId());

    ucontext_t* _ucontext = ucontext;

    fprintf(stdout, "PC: %p\n", (void*)REG(RIP, pc));
    REG(RIP, pc) = (uintptr_t)run;
    fprintf(stdout, "changed PC: %p\n", (void*)REG(RIP, pc));
}

SigAction installSignalHandler(int signo, SigAction action, SigHandler handler) {
    struct sigaction sa;
    struct sigaction oldsa;
    sigemptyset(&sa.sa_mask);

    if (handler != NULL) {
        sa.sa_handler = handler;
        sa.sa_flags = 0;
    } else {
        sa.sa_sigaction = action;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
    }

    if (sigaction(signo, &sa, &oldsa) != 0) {
        fprintf(stderr, "sigaction failed: %s\n", strerror(errno));
        return NULL;
    }
    return oldsa.sa_sigaction;
}

void test_signal_SIGSEGV() {
    installSignalHandler(SIGSEGV, signalHandler, NULL);
    pthread_t thrd1;
    if (pthread_create(&thrd1, NULL, run, NULL) != 0)
    {
        printf("thread error:%s \n", strerror(errno));
        return;
    }
    pthread_detach(thrd1);
}

void wait_signal()
{
    while (1) {
        char timeStamp[32];
        get_format_time_ms(timeStamp);
        fprintf(stdout, "%s, please press to exit %lu\n", timeStamp, getThreadId());
        unsigned int ret = sleep(1);
        if (ret != 0) {
            get_format_time_ms(timeStamp);
            printf("%s, sleep error:%s %d %lu\n", timeStamp, strerror(errno), ret, getThreadId());
        }
    }
}

int main()
{
    test_signal_SIGSEGV();
    wait_signal();

    return(0);
}
//
// Created by 李扬 on 2023/4/29.
//
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void get_format_time_ms(char *str_time);

int flag = 1;
int block = 1;

long getThreadId() {
    pthread_t cur = pthread_self();
    return (long)cur;
}

void *run() {
    if (block) {
        sigset_t mask, old_mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGFPE);  // 添加 SIGFPE 信号到掩码

        // 设置信号掩码，阻塞 SIGFPE
        pthread_sigmask(SIG_BLOCK, &mask, &old_mask);

        printf("Thread: SIGFPE is now blocked  %lu\n", getThreadId());
    }

    printf("start to trigger SIGFPE in few seconds %lu\n", getThreadId());
    sleep(3);

    int a = 1, b = 0, c;
    c = a / b;
    fprintf(stdout, "c = %d\n", c);

    // SIGSEGV 和 SIGFPE 是指令异常，无法从异常中恢复执行后续指令：
    // 要么在信号处理中退出程序(不然不停触发信号)
    // 要么信号被block后coredump终止程序
    // 要么siglongjmp跳转到其他指令处
    printf("recover from SIGFPE %lu\n", getThreadId());
    return NULL;
}

void signal_SIGFPE()
{
    char timeStamp[32];
    get_format_time_ms(timeStamp);
    fprintf(stdout, "%s caught SIGFPE signal %lu\n", timeStamp, getThreadId());
    exit(1);
}

void signal_SIGUSR1()
{
    char timeStamp[32];
    get_format_time_ms(timeStamp);
    fprintf(stdout, "%s caught SIGUSR1 signal %lu\n", timeStamp, getThreadId());
}

int test_signal_SIGFPE()
{
    if (signal(SIGFPE, signal_SIGFPE) == SIG_ERR) {
        fprintf(stdout, "cannot handle SIGFPE\n");
    } else {
        fprintf(stdout, "xxxxx\n");
    }

    pthread_t thrd1;
    if (pthread_create(&thrd1, NULL, run, NULL) != 0)
    {
        printf("thread error:%s \n", strerror(errno));
        return 1;
    }
    pthread_detach(thrd1);
    return 0;
}

void *wait_func(void* args) {
    int sig = SIGUSR1;
    printf("start to wait %d in few seconds %lu\n", sig, getThreadId());
    // 使用sigwait调用等待信号
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    // 这里有个竞态要处理：如果在sigwait调用之前就发出信号，那么会被信号handler抢跑，需要在创建线程前就让线程block信号(继承自父线程)，信号block后会被pending直到sigwait的调用(此时立即返回)
    int wait_ret = 0;
    sigwait(&set, &wait_ret);
    printf("wait signal %d %lu\n", wait_ret, getThreadId());
    return NULL;
}

int test_signal_SIGUSR1()
{
    int sig = SIGUSR1;
    if (signal(sig, signal_SIGUSR1) == SIG_ERR) {
        fprintf(stdout, "cannot handle SIGUSR1\n");
    } else {
        fprintf(stdout, "xxxxx\n");
    }

    pthread_t thrd1;
    if (pthread_create(&thrd1, NULL, wait_func, NULL) != 0)
    {
        printf("thread error:%s \n", strerror(errno));
        return 1;
    }
    pthread_detach(thrd1);
    return 0;
}

void wait_signal()
{
    if (block) {
        sigset_t mask, old_mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);  // 添加 SIGUSR1 信号到掩码

        // 设置信号掩码，阻塞 SIGUSR1
        pthread_sigmask(SIG_BLOCK, &mask, &old_mask);

        printf("Thread: SIGUSR1 is now blocked  %lu\n", getThreadId());
    }
    while (flag) {
        char timeStamp[32];
        get_format_time_ms(timeStamp);
        fprintf(stdout, "%s, please press to exit %lu\n", timeStamp, getThreadId());
        // 信号会中断某些耗时的系统调用，但c库做了重试，所以不会返回EINTR
        unsigned int ret = sleep(1); //sleep会被信号中断并返回0，大坑
        if (ret != 0) {
            get_format_time_ms(timeStamp);
            printf("%s, sleep error:%s %d %lu\n", timeStamp, strerror(errno), ret, getThreadId());
        }
    }
}

void main(int argc, char *argv[])
{
    block = (argc > 1) ? atoi(argv[1]) : 1;

    // test_signal_SIGFPE();
    test_signal_SIGUSR1();
    wait_signal();
}
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
#include <setjmp.h>

void get_format_time_ms(char *str_time);

int flag = 1;
jmp_buf env;

long getThreadId() {
    pthread_t cur = pthread_self();
    return cur->__sig;
}

void process_exit(int sig)
{
    char timeStamp[32];
    get_format_time_ms(timeStamp);
    switch (sig) {
        case SIGINT:
            fprintf(stderr, "%s,process exit: SIGINT: value: %d\n", timeStamp, sig);
            break;
        case SIGFPE:
            fprintf(stderr, "%s,process exit: SIGFPE: value: %d\n", timeStamp, sig);
            break;
        case SIGABRT:
            fprintf(stderr, "%s,process exit: SIGABRT: value: %d\n", timeStamp, sig);
            break;
        case SIGILL:
            fprintf(stderr, "%s,process exit: SIGILL: value: %d\n", timeStamp, sig);
            break;
        case SIGSEGV:
            fprintf(stderr, "%s,process exit: SIGSEGV: value: %d\n", timeStamp, sig);
            exit(1);//这里必须退出不然会不停产生SIGSEGV
        case SIGTERM:
            fprintf(stderr, "%s,process exit: SIGTERM: value: %d\n", timeStamp, sig);
            break;
        default:
            fprintf(stderr, "%s,process exit: value: %d\n", timeStamp, sig);
            break;
    }

    flag = 0;
}

void signal_type()
{
    signal(SIGINT, process_exit);
    signal(SIGFPE, process_exit);
    signal(SIGILL, process_exit);
    signal(SIGABRT, process_exit);
    signal(SIGSEGV, process_exit);
    signal(SIGTERM, process_exit);
}

void signal_sigill()
{
    fprintf(stdout, "caught SIGILL signal %lu\n", getThreadId());
}

void signal_sigterm()
{
    fprintf(stdout, "caught SIGTERM signal %lu\n", getThreadId());
}

void signal_sigsegv()
{
    fprintf(stdout, "caught SIGSEGV signal %lu\n", getThreadId());
    siglongjmp(env,1);//try catch 的原型
}



int test_signal_SIGINT()
{
    signal_type();

    return 0;
}

int test_signal_SIGILL()
{
    //signal_type();

    if (signal(SIGILL, signal_sigill) == SIG_ERR) {
        fprintf(stdout, "cannot handle SIGILL\n");
    } else {
        fprintf(stdout, "yyyyy\n");
    }

    return 0;
}

int test_signal_SIGFPE()
{
    signal_type();

    int a = 1, b = 0, c;
    c = a / b;
    fprintf(stdout, "c = %d\n", c);

    return 0;
}

void *run() {
    int r = sigsetjmp(env,1);
    if (r == 0) {
        printf("start to trigger SIGSEGV in few seconds %lu\n", getThreadId());
        sleep(7);
        int a[3] = {0};
        fprintf(stdout, "a[3] = %d\n", a[-1111111]);//trigger SIGSEGV
    } else {
        printf("recover from SIGSEGV %lu\n", getThreadId());
        run();
    }
    return NULL;
}

int test_signal_SIGSEGV()
{
    //signal_type();
    if (signal(SIGSEGV, signal_sigsegv) == SIG_ERR) {
        fprintf(stdout, "cannot handle SIGTERM\n");
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

int test_signal_SIGTERM()
{
    //signal_type();

    if (signal(SIGTERM, signal_sigterm) == SIG_ERR) {
        fprintf(stdout, "cannot handle SIGTERM\n");
    } else {
        fprintf(stdout, "xxxxx\n");
    }

    return 0;
}

int test_signal_SIGABRT()
{
    signal_type();

    abort();

    return 0;
}

void wait_signal()
{
    while (flag) {
        char timeStamp[32];
        get_format_time_ms(timeStamp);
        fprintf(stdout, "%s, please press to exit %lu\n", timeStamp, getThreadId());
        unsigned int ret = sleep(3);
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
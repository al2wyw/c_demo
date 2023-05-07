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
    fprintf(stdout, "caught SIGILL signal\n");
}

void signal_sigterm()
{
    fprintf(stdout, "caught SIGTERM signal\n");
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
    pthread_t cur = pthread_self();
    printf("start to trigger SIGSEGV in few seconds %lu\n", cur->__sig);
    sleep(7);
    int a[3] = {0};
    fprintf(stdout, "a[3] = %d\n", a[-1111111]);//trigger SIGSEGV
    return NULL;
}

int test_signal_SIGSEGV()
{
    signal_type();
    pthread_t thrd1;
    if (pthread_create(&thrd1, NULL, run, NULL) != 0)
    {
        printf("thread error:%s \n", strerror(errno));
        return 1;
    }
    pthread_detach(thrd1);
    //run();
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
        fprintf(stdout, "%s, please press to exit\n", timeStamp);
        unsigned int ret = sleep(3);
        if (ret != 0) {
            get_format_time_ms(timeStamp);
            printf("%s, sleep error:%s %d\n", timeStamp, strerror(errno), ret);
        }
    }
}

int main()
{
    test_signal_SIGINT();
    wait_signal();

    return(0);
}
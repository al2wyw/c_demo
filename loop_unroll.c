//
// Created by 李扬 on 2026/4/17.
//
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long long get_timestamp_ns();

const int LOOP = 100000;

#define OP *
#define data_type int


int test(data_type* arr, int loop) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i+=2) {
        x = x OP arr[i] OP arr[i+1];
    }
    printf("ret:%d, time: %lld\n", x, get_timestamp_ns() - start);
    return 0;
}


int test1(data_type* arr, int loop) { // 累积变量变换
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    data_type y = 0x1;
    for (int i = 0; i < loop; i+=2) {
        x = x OP arr[i];
        y = y OP arr[i+1];
    }
    x = x OP y;
    printf("ret:%d, time: %lld\n", x, get_timestamp_ns() - start);
    return 0;
}


int test2(data_type* arr, int loop) { // 重新结合变换
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i+=2) {
        x = x OP (arr[i] OP arr[i+1]);
    }
    printf("ret:%d, time: %lld\n", x, get_timestamp_ns() - start);
    return 0;
}

int main(int argc, char *argv[]) {
    int loop = (argc > 1) ? atoi(argv[1]) : LOOP;
    data_type* arr = malloc(loop * sizeof(data_type));
    if (arr == NULL) {
        printf("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    for (int i = 0; i < loop; i++) {
        arr[i] = rand() % loop;
    }
    test(arr, loop);
    test1(arr, loop);
    test2(arr, loop);
    free(arr);
    return 0;
}
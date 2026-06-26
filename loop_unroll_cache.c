//
// Created by 李扬 on 2026/4/17.
//
/**
 * 与 loop_unroll.c 的区别：
 *   loop_unroll.c 的数组大小 = 用户传入参数（例如 100000000 long = 800MB），远超 L3 cache，
 *   实测瓶颈在 DRAM 带宽上，导致 test0 / test1 / test2 / test3 几乎无差别。
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long long get_timestamp_ns();

#define OP +
#define data_type long

// L1 d-cache 大小（来自 lscpu：L1d cache = 32K）
#define L1_CACHE_SIZE (32 * 1024)
// 内层循环元素个数：让数组恰好占满 L1
#define LOOP (L1_CACHE_SIZE / (int)sizeof(data_type))

int base(data_type* arr, int repeat) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int r = 0; r < repeat; r++) {
        for (int i = 0; i < LOOP; i++) {
            x = x OP arr[i];
        }
    }
    printf("ret:%ld, time: %lld\n", (long)x, get_timestamp_ns() - start);
    return 0;
}

int test0(data_type* arr, int repeat) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int r = 0; r < repeat; r++) {
        for (int i = 0; i < LOOP; i+=4) {
            x = x OP arr[i];
            x = x OP arr[i+1];
            x = x OP arr[i+2];
            x = x OP arr[i+3];
        }
    }
    printf("ret:%ld, time: %lld\n", (long)x, get_timestamp_ns() - start);
    return 0;
}


int test1(data_type* arr, int repeat) { // 累积变量变换
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    data_type y = 0x0;
    data_type n = 0x0;
    data_type m = 0x0;
    for (int r = 0; r < repeat; r++) {
        for (int i = 0; i < LOOP; i+=4) {
            x = x OP arr[i];
            y = y OP arr[i+1];
            n = n OP arr[i+2];
            m = m OP arr[i+3];
        }
    }
    x = x OP y;
    n = n OP m;
    x = x OP n;
    printf("ret:%ld, time: %lld\n", (long)x, get_timestamp_ns() - start);
    return 0;
}

int test2(data_type* arr, int repeat) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int r = 0; r < repeat; r++) {
        for (int i = 0; i < LOOP; i+=4) {
            x = x OP arr[i] OP arr[i+1] OP arr[i+2] OP arr[i+3];
        }
    }
    printf("ret:%ld, time: %lld\n", (long)x, get_timestamp_ns() - start);
    return 0;
}


int test3(data_type* arr, int repeat) { // 重新结合变换
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int r = 0; r < repeat; r++) {
        for (int i = 0; i < LOOP; i+=4) {
            x = x OP ((arr[i] OP arr[i+1]) OP (arr[i+2] OP arr[i+3]));
        }
    }
    printf("ret:%ld, time: %lld\n", (long)x, get_timestamp_ns() - start);
    return 0;
}

int main(int argc, char *argv[]) {
    // 用户传入参数视为"总操作数"，除以 LOOP 得到外层 REPEAT 次数
    int total = (argc > 1) ? atoi(argv[1]) : (LOOP * 100000);
    int repeat = total / LOOP;
    if (repeat <= 0) repeat = 1;

    data_type* arr = malloc(LOOP * sizeof(data_type));
    if (arr == NULL) {
        printf("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    for (int i = 0; i < LOOP; i++) {
        arr[i] = rand() % LOOP;
    }
    printf("L1=%d bytes, sizeof(data_type)=%zu, LOOP=%d elements (%zu bytes), REPEAT=%d, total ops=%d\n",
           L1_CACHE_SIZE, sizeof(data_type), LOOP, LOOP * sizeof(data_type),
           repeat, LOOP * repeat);

    base(arr, repeat);
    test0(arr, repeat);
    test1(arr, repeat);
    test2(arr, repeat);
    test3(arr, repeat);
    free(arr);
    return 0;
}

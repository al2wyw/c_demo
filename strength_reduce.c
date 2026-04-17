//
// Created by 李扬 on 2026/4/17.
//
#include <stdio.h>
#include <stdlib.h>

long long get_timestamp_ns();

const int LOOP = 100000;

int test(register int h, int loop) {
    long long start = get_timestamp_ns();
    register int x = 0x10;
    for (int i = 0; i < loop; i++) {
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
        x += h;
    }
    printf("ret:%d, time: %lld\n", x, get_timestamp_ns() - start);
    return 0;
}

int test2(register int h, int loop) {
    long long start = get_timestamp_ns();
    register int x = 0x10;
    for (int i = 0; i < loop; i++) {
        x +=  h << 4;
    }
    printf("ret:%d, time: %lld\n", x, get_timestamp_ns() - start);
    return 0;
}

int main(int argc, char *argv[]) {
    int h = (argc > 1) ? atoi(argv[1]) : 10;
    int loop = (argc > 2) ? atoi(argv[2]) : LOOP;
    test(h, loop);
    test2(h, loop);
    return 0;
}
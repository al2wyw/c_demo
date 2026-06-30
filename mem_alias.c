//
// Created by 李扬 on 2026/5/10.
//
/*
内存读写涉及内存别名(写后读)时的cpu周期会加长
记得用O3优化，不然O1有太多的内存操作影响实验结果
*/
#include <stdio.h>
#include <stdlib.h>

void read_write(int* src, int* dst, int n) {
    int val = 0;
    for (int i = 0; i < n; i++) {
        *dst = val;
        val = *src + 1;
    }
}

#define CACHE_LINE_SIZE 64

typedef struct {
    char padding[CACHE_LINE_SIZE + 2];
} data_t;

void test_with_align(int check, int loop) {
    data_t* data = aligned_alloc(CACHE_LINE_SIZE, sizeof(data_t));

    int* p = (int*)&data->padding[0];
    int* q = (int*)&data->padding[CACHE_LINE_SIZE - 2];

    printf("%p, %p\n", p, q);
    read_write( p, check == 1 ? q : p, loop);
    printf("%d, %d\n", *p, *q);

    free(data);
}

int main(int argc, char *argv[]) {

    int loop = (argc > 1) ? atoi(argv[1]) : 10000;
    int check = (argc > 2) ? atoi(argv[2]) : 1;

    int i = 2;
    int j = 2;
    read_write( &i, check == 1 ? &j : &i, loop);
    printf("%d, %d\n", i, j);
    return 0;
}
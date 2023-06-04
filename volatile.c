//
// Created by 李扬 on 2023/5/30.
// gcc -g -O1 volatile.c -o volatile
// no-nv: no -O1 no volatile
// no: no -O1
// nv: no volatile
// yf: asm volatile ("":::"memory")
#include<stdio.h>
#include<stdlib.h>

#define mb() asm volatile ("":::"memory")

int val = 10;

void test(int loop) {
    for (int i = 0; i < loop; ++i) {
        // asm volatile ("leal 30(%1), %0":"=r"(val):"b"(val):);
        // asm volatile ("addl $30, %0":"=r"(val)::);
        mb();
        val+= 10;
    }

    printf("Hello, World! %d\n", val);
}
int main(int v, char **args) {
    int loop = atoi(args[1]);
    test(loop);
}

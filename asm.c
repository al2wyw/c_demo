//
// Created by root on 6/3/23.
// gcc -S store.c -> store.s
// gcc -c store.s -> store.o
// gcc -o asm asm.c store.o
#include <stdio.h>

//签名不一致也没有问题
int mul(int a, int b);

//被汇编代码调用, 即mul调用print
void print(long l) {
    long ret = l * 2;
    printf("print! %ld\n", ret);
}

int add() {
    int result= 0;
    int input = 1;
    __asm__ __volatile__ ("addl %1,%0":"=r"(result): "m"(input):"cc");// cc: code condition register

    printf("Hello, World! %d\n", result);
}

void play(int* i) {
    int j = 0;
    if (i!=0) {
        return;
    }
    printf("Hello, World! %x\n", j);
}

int main() {
    play(NULL);
    add();
    int ret = mul(4,6);
    printf("ret %d\n", ret);
}
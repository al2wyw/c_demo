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

void play(int* i) {
    int j = 0;
    if (i!=0) {
        return;
    }
    printf("Hello, World! %x\n", j);
}

int main() {
    play(NULL);
    int ret = mul(4,6);
    printf("ret %d\n", ret);
    printf("mul addr %p\n", mul);
}
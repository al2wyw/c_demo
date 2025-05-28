//
// Created by 李扬 on 2023/5/30.
// gcc -g -O1 volatile.c -o volatile
// no-nv: no -O1 no volatile
// no: no -O1
// nv: no volatile
// yf: asm volatile ("":::"memory")
// no-nv 和 no-yv 对比，yo-nv 和 yo-yv 对比，调查volatile和编译器优化对val的影响(此时没有mb):
// no-nv 和 no-yv 的代码基本一致；yo-nv 和 yo-yv的代码不一致，yo-nv把for循环展开了，yo-yv每次从内存读取val
// yo-yv 和 yo-yf 对比，调查volatile和mb的效果基本一致，val每次从内存读取
// 使用 volatile 修饰的变量，编译器不会对其进行代码优化，cpu不会将其缓存到寄存器和cpu cache中，保证执行时每次都从内存中读取(防优化、防缓存)
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

//
// Created by root on 6/3/23.
//
#include <stdio.h>

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
}
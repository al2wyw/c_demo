//
// Created by root on 6/3/23.
// strace -e write printf
// ref to printf.md
#include <stdio.h>

int main() {
    for (int i = 0; i < 10; i++) {
        printf("hi");
        //printf("hi\n");
    }
    printf("hi\n");
    return 0;
}
//
// Created by 李扬 on 2025/6/26.
//
#include <stdio.h>

// p &code
// disass /r 0x7fffffffe300, 0x7fffffffe310
int main() {
    char code[] = {
        0x55,                                       //pushq $rbp
        0x48, 0x89, 0xe5,                           //movq %rsp, %rbp
        0xb8, 0x0b, 0x00, 0x00, 0x00,               //movl $0xb, %eax
        0x5d,                                       //popq %rbp
        0xc3                                        //retq
    };
    printf("%d\n", code[0]);
    return 0;
}
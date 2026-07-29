//
// Created by 李扬 on 2026/7/29.
//
/*
 * gcc ref_main.c -L ./ -lref -o main
 * ref_val 通过内存偏移地址访问
 * lib_val 也是通过内存偏移地址访问，其GOT的地址就是其内存偏移地址
 */
#include "ref.h"

void print_ref_val();
extern int lib_val;

int main(int argc, char *argv[]) {
    ref_val++;
    lib_val++;
    print_ref_val();
    return 0;
}
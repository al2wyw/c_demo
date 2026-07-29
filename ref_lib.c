//
// Created by 李扬 on 2026/7/29.
//
/*
 * gcc -fPIC -shared ref_lib.c -o libref.so
 * export LD_LIBRARY_PATH=/path_to_lib
 * -fPIC 使得 ref_val lib_val 的访问会通过GOT
 */
#include <stdio.h>
#include "ref.h"

int lib_val = 10;

void print_ref_val() {
    printf("ref_val: %d\n", ref_val);
    printf("lib_val: %d\n", lib_val);
}
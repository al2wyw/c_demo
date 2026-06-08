//
// Created by 李扬 on 2026/6/3.
//
#ifdef __bmi2__
#include <x86gprintrin.h>
#endif
#include <stdio.h>

unsigned int has_x_byte(unsigned int x, unsigned char y) {
    x = x ^ (y * 0x01010101U);
    return (x - 0x01010101) & ~x & 0x80808080;
}

unsigned long has_x_byte_l(unsigned long x, unsigned char y) {
    x = x ^ ((unsigned long)y * 0x0101010101010101UL);
    return (x - 0x0101010101010101UL) & ~x & 0x8080808080808080UL;
}

unsigned int has_zero_byte(unsigned int x) {
    return (x - 0x01010101) & ~x & 0x80808080;
}

unsigned long has_zero_byte_l(unsigned long x) {
    return (x - 0x0101010101010101UL) & ~x & 0x8080808080808080UL;
}

static const int Mod37BitPosition[] = // map a bit value mod 37 to its position
    {
    32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
    7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
    20, 8, 19, 18
  };

int get_padding_zero(unsigned int v) {
    return Mod37BitPosition[(-v & v) % 37];
}

int get_padding_zero_l(unsigned long v) {
    int r = get_padding_zero(v & 0xffffffff);
    return r == 32 ? get_padding_zero(v >> 32) + r : r;
}
/*
int get_padding_zero_l(unsigned long v) {
    unsigned int low = v & 0xffffffff;
    return low == 0 ? get_padding_zero(v >> 32) + 32 : get_padding_zero(low);
}
 */

// *************** bit perf test ******************
// bmi: bit manipulation instruction
__attribute__((target("bmi2")))
unsigned int zero_byte(unsigned int x) {
#ifdef __bmi2__
    unsigned int r = (x - 0x01010101) & ~x & 0x80808080;
    // 让一部分用 BMI2 的 PEXT（如果机器支持）
    return  _pext_u32(r, 0x80808080);
#else
    return 0;
#endif
}

unsigned int zero_byte_0(unsigned int x) {
    unsigned int r = (x - 0x01010101) & ~x & 0x80808080;
    // 直接把 4 个 sign bit 收成一个 4bit mask（避免逐个移位）
    // r 只有第 7/15/23/31 位可能是 1
    unsigned int m = ((r * 0x00204081U) >> 28) & 0xF;  // 把 4 个 sign 位塞到低 4 bit
    return m;
}

unsigned int zero_byte_1(unsigned int x) {
    unsigned int ret = (x - 0x01010101) & ~x & 0x80808080;
    if (ret == 0) {
        return 0;
    }
    unsigned int ret1 = (ret & 0xff) >> 7 ;
    unsigned int ret2 = (ret >> 8 & 0xff) >> 6;
    unsigned int ret3 = (ret >> 16 & 0xff) >> 5;
    unsigned int ret4 = (ret >> 24 & 0xff) >> 4;
    ret1 |= ret2;
    ret3 |= ret4;
    return ret1 | ret3;
}

unsigned int zero_byte_2(unsigned int x) {
    unsigned int ret = (x - 0x01010101) & ~x & 0x80808080;
    unsigned int ret2 = 0;
    int i = 7;
    while (ret != 0) {
        ret2 |= (ret & 0xff) >> i;
        ret >>= 8;
        i--;
    }
    return ret2;
}

// 阻止内联，call zero_byte_1 就会回来
// 把 zero_byte_1 改成：
//   __attribute__((noinline)) unsigned int zero_byte_1(...)
// 或者用 volatile 强制使用返回值：
//   volatile unsigned int sink;
//   sink = zero_byte_1(0x01020300);
#pragma GCC push_options
#pragma GCC optimize ("O0")
void perf_test(){
    unsigned int target = 0x01020300;
    // 0x01020300 zero_byte_2的while只要运行一次就结束，zero_byte_1无论如何都要把指令全部运行完
    // target每次都改变，while的分支预测被打乱，性能就会变差
    for (int i = 0; i < 1000000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte_1(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }

    //分支预测命中高的情况下更优，用perf stat查看branch-miss和ipc，用perf record只能查看热点，看不出branch-miss的影响
    for (int i = 0; i < 1000000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte_2(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }
    // llvm-mca 生成的 Critical sequence 还是没搞明白
}
#pragma GCC pop_options
// *************** bit perf test ******************

int main() {
    printf("%u\n", has_zero_byte(0x01020304));
    printf("%u\n", has_zero_byte(0x01020300));
    printf("%u\n", has_zero_byte(0x00020300));

    printf("%lu\n", has_zero_byte_l(0x0102030405060708UL));
    printf("%lu\n", has_zero_byte_l(0x0102030005060708UL));
    printf("%lu\n", has_zero_byte_l(0x0002030005060708UL));

    printf("%u\n", has_x_byte(0x01020304, 0x05));
    printf("%u\n", has_x_byte(0x01020305, 0x05));
    printf("%u\n", has_x_byte(0x05020305, 0x05));

    printf("%lu\n", has_x_byte_l(0x0102030405060708UL, 0x09));
    printf("%lu\n", has_x_byte_l(0x0102030905060708UL, 0x09));
    printf("%lu\n", has_x_byte_l(0x0902030905060708UL, 0x09));

    printf("%d\n", get_padding_zero_l(0x0102030405060708UL));
    printf("%d\n", get_padding_zero_l(0x0102030405060700UL));
    printf("%d\n", get_padding_zero_l(0x0102030405060000UL));

    unsigned int target = 0x01020300;
    for (int i = 0; i < 16; i++) {
        printf("zero_byte_n: %u\n", zero_byte(target));
        printf("zero_byte_0: %u\n", zero_byte_0(target));
        printf("zero_byte_1: %u\n", zero_byte_1(target));
        printf("zero_byte_2: %u\n", zero_byte_2(target));
        target =  (target & 0xff) << 24 | target >> 8;
    }

    perf_test();
    return 0;
}
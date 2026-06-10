//
// Created by 李扬 on 2026/6/3.
//
#ifdef __bmi2__
#include <x86gprintrin.h>
#endif
#include <stdio.h>

#define has_zero_byte(x) ((x - 0x01010101) & ~x & 0x80808080)
#define has_zero_byte_l(x) ((x - 0x0101010101010101UL) & ~x & 0x8080808080808080UL)
#define zero_byte_def(ret)  unsigned int ret1 = (ret & 0xff) >> 7 ; \
                            unsigned int ret2 = (ret >> 8 & 0xff) >> 6; \
                            unsigned int ret3 = (ret >> 16 & 0xff) >> 5; \
                            unsigned int ret4 = (ret >> 24 & 0xff) >> 4; \
                            ret1 |= ret2; \
                            ret3 |= ret4; \
                            ret1 |= ret3;

// *************** bit perf test ******************
// bmi: bit manipulation instruction
__attribute__((target("bmi2")))
unsigned int zero_byte(unsigned int x) {
#ifdef __bmi2__
    unsigned int r = has_zero_byte(x);
    // 让一部分用 BMI2 的 PEXT（如果机器支持）
    return  _pext_u32(r, 0x80808080);
#else
    return 0;
#endif
}

//0x00204081 = 2^0 + 2^7 + 2^14 + 2^21，每个 1 间隔 7 位。
//r 中在 sign bit位 7/15/23/31 上的 1 间隔 8 位.
//为了让 r 的 1 间隔 1 位，8 - 1 = 7 (不确定对不对)
unsigned int zero_byte_0(unsigned int x) {
    unsigned int r = has_zero_byte(x);
    // 直接把 4 个 sign bit 收成一个 4bit mask（避免逐个移位）
    // r 只有第 7/15/23/31 位可能是 1
    unsigned int m = ((r * 0x00204081U) >> 28) & 0xF;  // 把 4 个 sign 位塞到低 4 bit
    return m;
}

unsigned int zero_byte_0_l(unsigned long x) {
    // 64 位版本：x 中可能为 1 的 sign bit 位于 bit 7/15/23/31/39/47/55/63（共 8 个，间距 8）
    // 用魔数 M = 2^0 + 2^7 + 2^14 + 2^21 + 2^28 + 2^35 + 2^42 + 2^49 = 0x0002040810204081
    // 每个 1 之间间隔 7 位，乘法后把 8 个 sign bit 正好"挤"到结果的最高 8 位 [63:56]，且互不冲突
    unsigned long r = has_zero_byte_l(x);
    unsigned int m = (unsigned int)((r * 0x0002040810204081UL) >> 56) & 0xFF;
    return m;
}

unsigned int zero_byte_1(unsigned int x) {
    unsigned int ret = has_zero_byte(x);
    zero_byte_def(ret);
    return ret1;
}

unsigned int zero_byte_2(unsigned int x) {
    unsigned int ret = has_zero_byte(x);
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
    //查看cpu资源端口压力 (zero_byte_1 的 port_0 + port_6 利用率接近 100%, 被4个shr操作撑满)
    //perf stat -e cycles,instructions,uops_dispatched_port.port_0,uops_dispatched_port.port_6,uops_dispatched_port.port_1,uops_dispatched_port.port_5,branches,branch-misses

    // 0x01020300 zero_byte_2的while只要运行一次就结束，zero_byte_1无论如何都要把指令全部运行完
    // target每次都改变，while的分支预测被打乱，性能就会变差
    for (int i = 0; i < 10000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte_1(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }

    //分支预测命中高的情况下更优，用perf stat查看branch-miss和ipc，用perf record只能查看热点，看不出branch-miss的影响
    for (int i = 0; i < 10000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte_2(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }

    for (int i = 0; i < 10000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte_0(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }
    for (int i = 0; i < 10000000; i++) {
        volatile unsigned int sink;
        sink = zero_byte(target);
        //target =  (target & 0xff) << 24 | target >> 8;
    }
}
#pragma GCC pop_options

// gcc -S -O3 bit_twiddling_perf.c -o bit.s
// llvm-mca -timeline -bottleneck-analysis bit.s
// llvm-mca 必须应用在循环上，不能有ret和call跳转:
// # LLVM-MCA-BEGIN
// Loop:
// # ...
// jb .Loop
// # LLVM-MCA-END
void zero_byte_1_mca() {
    for (int i = 0; i < 10000000; i++) {
        __asm volatile("# LLVM-MCA-BEGIN zero_byte_1":::"memory");
        unsigned int x = 0x01020300;
        unsigned int ret = has_zero_byte(x);
        zero_byte_def(ret);
        volatile unsigned int sink = ret1;
        __asm volatile("# LLVM-MCA-END":::"memory");
    }
}
// *************** bit perf test ******************

// gcc -O3 bit_twiddling_perf.c -D__bmi2__ -o bit_perf_o3
int main() {
    unsigned int target = 0x01020300;
    for (int i = 0; i < 16; i++) {
        printf("zero_byte_n: %u\n", zero_byte(target));
        printf("zero_byte_0: %u\n", zero_byte_0(target));
        printf("zero_byte_1: %u\n", zero_byte_1(target));
        printf("zero_byte_2: %u\n", zero_byte_2(target));
        target =  (target & 0xff) << 24 | target >> 8;
    }

    unsigned long target_l = 0x0102030004050607UL;
    for (int i = 0; i < 16; i++) {
        printf("zero_byte_0_l: %u\n", zero_byte_0_l(target_l));
        target_l = (target_l & 0xff) << 56 | target_l >> 8;
    }

    perf_test();
    return 0;
}
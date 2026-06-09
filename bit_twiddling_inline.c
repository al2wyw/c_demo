//
// Created by 李扬 on 2026/6/3.
//
#ifdef __bmi2__
#include <x86gprintrin.h>
#endif
#include <stdio.h>

// *************** bit perf test ******************
// 用 static inline + always_inline 强制内联进 perf_test，免除 call/ret 开销，
// 同时避免裸 inline 在 -O0 调用方下产生 undefined symbol 链接错误
#define ALWAYS_INLINE static inline __attribute__((always_inline))

// bmi: bit manipulation instruction
__attribute__((target("bmi2")))
ALWAYS_INLINE unsigned int zero_byte(unsigned int x) {
#ifdef __bmi2__
    unsigned int r = (x - 0x01010101) & ~x & 0x80808080;
    // 让一部分用 BMI2 的 PEXT（如果机器支持）
    return  _pext_u32(r, 0x80808080);
#else
    return 0;
#endif
}

ALWAYS_INLINE unsigned int zero_byte_0(unsigned int x) {
    unsigned int r = (x - 0x01010101) & ~x & 0x80808080;
    // 直接把 4 个 sign bit 收成一个 4bit mask（避免逐个移位）
    // r 只有第 7/15/23/31 位可能是 1
    unsigned int m = ((r * 0x00204081U) >> 28) & 0xF;  // 把 4 个 sign 位塞到低 4 bit
    return m;
}

ALWAYS_INLINE unsigned int zero_byte_1(unsigned int x) {
    unsigned int ret = (x - 0x01010101) & ~x & 0x80808080;
    unsigned int ret1 = (ret & 0xff) >> 7 ;
    unsigned int ret2 = (ret >> 8 & 0xff) >> 6;
    unsigned int ret3 = (ret >> 16 & 0xff) >> 5;
    unsigned int ret4 = (ret >> 24 & 0xff) >> 4;
    ret1 |= ret2;
    ret3 |= ret4;
    return ret1 | ret3;
}

ALWAYS_INLINE unsigned int zero_byte_2(unsigned int x) {
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

// 反 DCE：告诉编译器“这个值被使用了”，但不产生任何真实指令
#define DO_NOT_OPTIMIZE_AWAY(x) __asm__ volatile("" :: "r"(x) : "memory")
// 反常量折叠：把变量“洗”一遍，让编译器把它当成未知值
#define ESCAPE(x)               __asm__ volatile("" : "+r"(x))

// 让 perf_test 走 -O3，配合 always_inline，被测函数会被真正内联进循环体，不再产生 call/ret 开销；
// 用 ESCAPE 阻止常量折叠、DO_NOT_OPTIMIZE_AWAY 阻止 DCE，这样既能内联又不会被整段消除。
void zero_byte_1_driver(){
    unsigned int target = 0x01020300;
    ESCAPE(target);  // 把 target 变成编译器“看不穿”的值，阻止常量传播

    for (int i = 0; i < 10000000; i++) {
        unsigned int x = target;
        ESCAPE(x);                         // 每轮都洗一次，阻止把循环外提
        unsigned int r = zero_byte_1(x);   // 因 always_inline，这里直接展开成位运算
        DO_NOT_OPTIMIZE_AWAY(r);           // 阻止结果被 DCE，但不产生真实指令
    }
}

void zero_byte_2_driver(){
    unsigned int target = 0x01020300;
    ESCAPE(target);

    for (int i = 0; i < 10000000; i++) {
        unsigned int x = target;
        ESCAPE(x);
        unsigned int r = zero_byte_2(x);
        DO_NOT_OPTIMIZE_AWAY(r);
    }
}

void zero_byte_0_driver(){
    unsigned int target = 0x01020300;
    ESCAPE(target);

    for (int i = 0; i < 10000000; i++) {
        unsigned int x = target;
        ESCAPE(x);
        unsigned int r = zero_byte_0(x);
        DO_NOT_OPTIMIZE_AWAY(r);
    }
}

// 注意：zero_byte 带 target("bmi2")，把它 always_inline 进来，调用方必须也具备同等或更强的target 选项
__attribute__((target("bmi2")))
void zero_byte_driver(){
    unsigned int target = 0x01020300;
    ESCAPE(target);

    for (int i = 0; i < 10000000; i++) {
        unsigned int x = target;
        ESCAPE(x);
        unsigned int r = zero_byte(x);
        DO_NOT_OPTIMIZE_AWAY(r);
    }
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
void perf_test(){
    zero_byte_1_driver();
    zero_byte_2_driver();
    zero_byte_0_driver();
    zero_byte_driver();
}
#pragma GCC pop_options
/*
+   98.48%     0.00%  bit_inline_o3  bit_inline_o3         [.] main                                                                     ◆
+   36.04%    36.04%  bit_inline_o3  bit_inline_o3         [.] zero_byte_1_driver                                                       ▒
+   32.49%    32.49%  bit_inline_o3  bit_inline_o3         [.] zero_byte_2_driver                                                       ▒
+   15.23%    15.23%  bit_inline_o3  bit_inline_o3         [.] zero_byte_0_driver                                                       ▒
+   14.72%    14.72%  bit_inline_o3  bit_inline_o3         [.] zero_byte_driver
 */
// *************** bit perf test ******************

// gcc -O3 bit_twiddling_inline.c -D__bmi2__ -o bit_inline_o3
int main() {
    perf_test();
    return 0;
}
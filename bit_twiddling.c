//
// Created by 李扬 on 2026/6/3.
//

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
    return 0;
}
//
// Created by 李扬 on 2026/4/27.
//
// 1. 内存对齐(位宽 vs 对齐):
// SSE	    128位        16字节
// AVX	    256位	    32字节
// AVX-512	512位	    64字节
// float a[len] __attribute__((aligned(16)));
// float b[len] __attribute__((aligned(16)));
// float res[len] __attribute__((aligned(16)));
// 2. 不要混用不同位宽的SIMD指令
// 查看cpu支持的指令集: cat /proc/cpuinfo | grep -o -E 'avx[0-9a-z]*' | sort -u
// gcc -std=c11 -O2 -mavx -mavx512f simd.c -o simd
// -mavx : 启用 __m256 及 _mm256_* | -mavx512f : 启用 __m512 及 _mm512_*
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>
#include <immintrin.h>

long long get_timestamp_ns();

void scalar(float* a, float* b, float* result, int size) {
    for (int i = 0; i < size; i++) {
        result[i] = a[i] * b[i];
    }
}

void simd4(float* a, float* b, float* res, int size) {
    for (int i = 0; i < size; i += 4) {
        __m128 A = _mm_load_ps(a + i);
        __m128 B = _mm_load_ps(b + i);
        __m128 RES = _mm_mul_ps(A, B);

        _mm_store_ps(res + i, RES);
    }
}

//可以免去 -mavx
__attribute__((target("avx")))
void simd8(float* a, float* b, float* res, int size) {
    for (int i = 0; i < size; i += 8) {
        __m256 A = _mm256_load_ps(a + i);
        __m256 B = _mm256_load_ps(b + i);
        __m256 RES = _mm256_mul_ps(A, B);

        _mm256_store_ps(res + i, RES);
    }
}

//可以免去 -mavx512f
__attribute__((target("avx512f")))
void simd16(float* a, float* b, float* res, int size) {
    for (int i = 0; i < size; i += 16) {
        __m512 A = _mm512_load_ps(a + i);
        __m512 B = _mm512_load_ps(b + i);
        __m512 RES = _mm512_mul_ps(A, B);

        _mm512_store_ps(res + i, RES);
    }
}

void init_data(float* a, float* b, int size) {
    for (int i = 0; i < size; i++) {
        a[i] = i;
        b[i] = i + 1;
    }
}

void* allocate(int size, int aligned) {
    if (aligned > 0) {
        void* ptr = aligned_alloc(aligned, size);
        printf("aligned: %d, %p\n", aligned, ptr);
        return ptr;
    }
    void* ptr = malloc(size);
    printf("malloc: %p\n", ptr);
    return ptr;
}

void scalar_test(int len, int loop) {
    float *a = allocate(sizeof(float) * len, 0);
    float *b = allocate(sizeof(float) * len, 0);
    float *res = allocate(sizeof(float) * len, 0);
    init_data(a, b, len);

    long long start = get_timestamp_ns();
    for (int i = 0; i < loop; i++) {
        scalar(a, b, res, len);
    }
    printf("time: %lld\n", get_timestamp_ns() - start);
    printf("%f, %f, %f, %f\n", res[0], res[1], res[2], res[3]);

    free(a);
    free(b);
    free(res);
}

void simd4_test(int len, int loop) {

    float *a = allocate(sizeof(float) * len, 16);
    float *b = allocate(sizeof(float) * len, 16);
    float *res = allocate(sizeof(float) * len, 16);

    init_data(a, b, len);

    long long start = get_timestamp_ns();
    for (int i = 0; i < loop; i++) {
        simd4(a, b, res, len);
    }
    printf("time: %lld\n", get_timestamp_ns() - start);
    printf("%f, %f, %f, %f\n", res[0], res[1], res[2], res[3]);

    free(a);
    free(b);
    free(res);
}

void simd8_test(int len, int loop) {

    float *a = allocate(sizeof(float) * len, 32);
    float *b = allocate(sizeof(float) * len, 32);
    float *res = allocate(sizeof(float) * len, 32);

    init_data(a, b, len);

    long long start = get_timestamp_ns();
    for (int i = 0; i < loop; i++) {
        simd8(a, b, res, len);
    }
    printf("time: %lld\n", get_timestamp_ns() - start);
    printf("%f, %f, %f, %f\n", res[0], res[1], res[2], res[3]);

    free(a);
    free(b);
    free(res);
}

void simd16_test(int len, int loop) {

    float *a = allocate(sizeof(float) * len, 64);
    float *b = allocate(sizeof(float) * len, 64);
    float *res = allocate(sizeof(float) * len, 64);

    init_data(a, b, len);

    long long start = get_timestamp_ns();
    for (int i = 0; i < loop; i++) {
        simd16(a, b, res, len);
    }
    printf("time: %lld\n", get_timestamp_ns() - start);
    printf("%f, %f, %f, %f\n", res[0], res[1], res[2], res[3]);

    free(a);
    free(b);
    free(res);
}


int main(int argc, char *argv[]) {
    int len = (argc > 1) ? atoi(argv[1]) : 10;
    int loop = (argc > 2) ? atoi(argv[2]) : 160000;
    if (len % 16 != 0) {
        printf("len must be a multiple of 16\n");
        return -1;
    }

    scalar_test(len, loop);
    simd4_test(len, loop);
    simd8_test(len, loop);
    simd16_test(len, loop);
    return 0;
}
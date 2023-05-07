#include <stdio.h>
#include <stdlib.h>

int* ptr = NULL;

int64_t mul(int64_t a, int64_t b);

int seg();

void func() {
    int i = 10;
    ptr = &i;
}

void funcOverflow() {
    int64_t h = 111343282;
}

int64_t func_mul(int64_t a, int64_t b, int64_t c) {
    a += c;
    return mul(b, a) + 10;
}

void funcExpandStack() {
    int64_t si = func_mul(34, 23, 1);
    int arr[si];
    arr[0] = 1;
}

typedef struct {
    int h;
} test, *testPtr;

struct myStruct {
    int i;
    char c;
    char d;
    long j;
};

struct myStruct2 {
    short w[3];
    char c[3];
};

struct myStruct3 {
    struct myStruct2 w[2];
    struct myStruct t;
};

void func_size() {
    struct myStruct t;
    printf("size! %ld, %p, %p, %p, %p\n", sizeof(t), &t.i, &t.c, &t.d, &t.j);
    struct myStruct2 t2;
    printf("size! %ld, %p, %p\n", sizeof(t2), &t2.w, &t2.c);
    struct myStruct3 t3;
    printf("size! %ld, %p, %p, %p\n", sizeof(t3), &t3.w, &t3.w[1], &t3.t);
}

int main() {
    //seg();
    func();
    funcOverflow();
    printf("Hello, World! %d\n", *(ptr-1));

    printf("Hello, World! %lld\n", mul(23, 34));

    u_int64_t h = 0x0101010101010101;
    h |= 0xff;
    printf("Hello, World! %lld\n", h);

    funcExpandStack();

    func_size();

    char c = -128;
    printf("Hello, World! %x\n", c);

    test i = {10};
    testPtr j = &i;

    exit(0);
}

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

    char c = -128;
    printf("Hello, World! %x\n", c);

    test i = {10};
    testPtr j = &i;

    exit(0);
}

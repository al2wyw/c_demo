//
// Created by 李扬 on 2025/6/6.
//
# include <stdio.h>

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
    func_size();
}
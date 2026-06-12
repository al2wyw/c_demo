//
// Created by 李扬 on 2026/6/12.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    void *p1 = malloc(32);
    void *p2 = malloc(128);
    char *s = strdup("hello world"); // strdup 内部也会调用 malloc

    printf("p1=%p p2=%p s=%s\n", p1, p2, s);

    free(p1);
    free(p2);
    free(s);
    return 0;
}
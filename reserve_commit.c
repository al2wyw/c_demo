//
// Created by 李扬 on 2025/8/20.
//

#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <memory.h>

void main() {
    // reserved
    void *temp = mmap (
        NULL,
        getpagesize(),
        PROT_NONE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1, //fd
        0 //offset
    );

    if (temp == NULL) {
        printf("mmap error: %s\n", strerror(errno));
        return;
    }

    printf("mmap address: %p\n", temp);

    // memset(temp, 0, getpagesize()); // SIGBUS

    // committed
    temp = mmap (
        temp,
        getpagesize(),
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
        -1, //fd
        0 //offset
    );

    if (temp == NULL) {
        printf("mmap error: %s\n", strerror(errno));
        return;
    }

    printf("mmap address: %p\n", temp);

    memset(temp, 0, getpagesize());

    printf("done\n");

}
//
// Created by 李扬 on 2025/11/17.
//

#include <stdio.h>
#include <stdlib.h>

// 用bpftrace可抓到每次Page Fault的地址
// 主缺页(swap,非常影响性能) 次缺页(物理内存映射)
// perf stat -e page-faults // perf list

int const PAGE_SIZE = 4096;

void main() {
    int loop = 10000;
    char *buf = malloc(PAGE_SIZE * loop); //只分配虚拟空间
    printf("buf: %p\n", buf);
    for (int i = 0; i < loop; i++) {
        //buf[0] = 'A';           // 第一次写入，Page Fault分配物理页
        //buf[PAGE_SIZE] = 'B';   // 跨页访问，再次Page Fault
        buf[i * PAGE_SIZE] = 'A'; // touch memory
    }

    free(buf);
}

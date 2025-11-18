//
// Created by 李扬 on 2025/11/17.
//

#include <stdlib.h>

// 用bpftrace可抓到每次Page Fault的地址
// 主缺页(swap,非常影响性能) 次缺页(物理内存映射)

int const PAGE_SIZE = 4096;

void main() {

    char *buf = malloc(PAGE_SIZE * 10); //只分配虚拟空间
    buf[0] = 'A';           // 第一次写入，Page Fault分配物理页
    buf[PAGE_SIZE] = 'B';   // 跨页访问，再次Page Fault

    free(buf);
}

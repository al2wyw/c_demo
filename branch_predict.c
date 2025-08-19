//
// Created by 李扬 on 2025/8/18.
//
#include <stdio.h>
#include <stdlib.h>

int compareInt(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

long long get_timestamp_ms();

int main() {
    // 随机产生整数，用分区函数填充，以避免出现分桶不均
    const unsigned arraySize = 32768;
    int data[arraySize];

    for (unsigned c = 0; c < arraySize; ++c)
        data[c] = rand() % 256;

    // !!! 排序后下面的Loop运行将更快
    qsort(data, arraySize, sizeof(int), compareInt);

    /*
    // opt: 性能提升10%
    for (unsigned c = 0; c < arraySize; ++c)
    {
        if (data[c] < 128)
            data[c] = 0;
    }
     */

    // 测试部分
    long long start = get_timestamp_ms();
    long long sum = 0;

    for (unsigned i = 0; i < 100000; ++i)
    {
        // 主要计算部分，选一半元素参与计算
        for (unsigned c = 0; c < arraySize; ++c)
        {
            if (data[c] >= 128)
                sum += data[c];
        }
        /*
        // opt:
        for (unsigned c = 0; c < arraySize; ++c)
        {
            sum += data[c];
        }
         */
    }

    double elapsedTime = (get_timestamp_ms() - start) / 1000.0;

    printf("%lf\n", elapsedTime);
    printf("%ld\n", sum);
}

/*
//使用 %edi 来代替 -40(%rbp), 性能提升20%
for (unsigned c = 0; c < arraySize; ++c)
{
    sum += data[c];
}
.LBB5:
    .loc 1 36 0
        movl    $0, %edi
    jmp	.L10
.L11:
    .loc 1 38 0 discriminator 2
    movq	-64(%rbp), %rax
    movl	%edi, %edx
    movl	(%rax,%rdx,4), %eax
    cltq
    addq	%rax, -32(%rbp)
    .loc 1 36 0 discriminator 2
    addl	$1, %edi
.L10:
    .loc 1 36 0 is_stmt 0 discriminator 1
    cmpl	-44(%rbp), %edi
    jb	.L11
 */
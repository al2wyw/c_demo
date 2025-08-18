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
    }

    double elapsedTime = (get_timestamp_ms() - start) / 1000.0;

    printf("%lf\n", elapsedTime);
    printf("%ld\n", sum);
}

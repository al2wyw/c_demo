//
// Created by 李扬 on 2025/11/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// perf stat -e L1-dcache-prefetch-misses,L1-dcache-load-misses,L1-dcache-store-misses,dTLB-load-misses,dTLB-loads
// cat /sys/devices/system/cpu/cpu1/cache/index0/coherency_line_size 64 byte
// false share 只出现在多线程场景(由MESI协议触发导致缓存行失效而从主存加载数据)，单线程无需考虑此问题

#define MAX 0xfffff

unsigned int next_rnd(unsigned int seed)
{
	return seed * 1664525 + 1013904223;
}

typedef struct data {
	// int tmp[15]; // 无需考虑false share
	int val;
} data_t;

data_t arr[MAX];
int rand_index[MAX];
int main(int argc, char **argv)
{
	int i, j, size;

	printf("data size is %ld\n", sizeof(data_t));

	size = atoi(argv[1]);

	int seed = time(NULL);;
	for(i = 0; i < size; ++i) {
		seed = next_rnd(seed)%size;
		rand_index[i] = seed;
	}

	if(atoi(argv[2]) == 0) { // 顺序访问内存
		for(i = 0; i < size; ++i) {
			for(j = 0; j < size; ++j) {
				seed = i;
				int tmp = arr[seed].val;
				// arr[seed].val = 1;
			}
		}
	} else { // 随机访问内存
		for(i = 0; i < size; ++i) {
			for(j = 0; j < size; ++j) {
				seed = rand_index[i];
				int tmp = arr[seed].val;
				// arr[seed].val = 1;
			}
		}
	}

	return 0;
}

//
// Created by 李扬 on 2025/11/19.
//

#include <stdlib.h>
#include <time.h>

// cat /sys/devices/system/cpu/cpu1/cache/index0/coherency_line_size

#define MAX 0xfffff

unsigned int next_seq(unsigned int seed)
{
	return seed * 1 + 11;
}

unsigned int next_rnd(unsigned int seed)
{
	return seed * 1664525 + 1013904223;
}

int arr[MAX];
int main(int argc, char **argv)
{
	int i, j, size;
	int seed = 0;

	size = atoi(argv[1]);
	if(atoi(argv[2]) == 0) { // 顺序访问内存
		for(i = 0; i < size; ++i) {
			seed = next_seq(seed)%size;
			j = arr[seed];
			// arr[seed] = 1;
		}
	} else { // 随机访问内存
		int seed = time(NULL);;
		for(i = 0; i < size; ++i) {
			seed = next_rnd(seed)%size;
			j = arr[seed];
			// arr[seed] = 1;
		}
	}

	return 0;
}

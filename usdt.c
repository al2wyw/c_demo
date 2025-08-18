//
// Created by 李扬 on 2025/8/18.
//

#include <sys/sdt.h>
#include <stdio.h>
#include <unistd.h>

int add(int a, int b) {
	DTRACE_PROBE(hello_usdt, probe_main);

	return a + b;
}

int main() {

    while (1) {
    	printf("result is %d\n", add(1, 2));
		sleep(1);
    }
    return 0;
}
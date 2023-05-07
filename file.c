//
// Created by 李扬 on 2023/4/24.
//
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

long long get_timestamp_ms();

void file() {
    FILE *pf = fopen("test.txt", "w+");

    if (pf == NULL) {
        printf("pf is null %s\n", strerror(errno));
        return;
    }

    const char *buf = "Don't give up and don't give in.";
    long long start = get_timestamp_ms();
    write(fileno(pf), buf, strlen(buf));
    //输入一个字符
    fputs("test", pf);
    fputc('a', pf);
    //用完关闭文件
    fclose(pf);
    printf("done %d ms\n", (int)(get_timestamp_ms() - start));
}

int main() {
    file();
}
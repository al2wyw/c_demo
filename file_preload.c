//
// Created by 李扬 on 2025/11/18.
//


#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// cachetop yum install -y bcc-tools linux4.1+
// pcstat go install github.com/tobert/pcstat@latest
// sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'

void main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return;
    }

    const char *filename = argv[1];
    printf("to open file %s\n", filename);

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file %s\n", strerror(errno));
        return;
    }
    struct stat file_state;
    if (fstat(fd, &file_state) == -1) { // 获取文件状态
        fprintf(stderr, "Error getting file status %s\n", strerror(errno));
        close(fd);
        return;
    }
    int size = file_state.st_size;
    char *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == NULL) {
        fprintf(stderr, "mmap error: %s\n", strerror(errno));
        close(fd);
        return;
    }
    // just advise, knernel may reject
    if (madvise(addr, size, MADV_WILLNEED) == -1) {
        fprintf(stderr, "advise will need failed %s\n", strerror(errno));
        close(fd);
        return;
    }

    for (size_t i = 0; i < size; ++i) {
        char c =addr[i];
        // putchar(addr[i]);
    }

    if (munmap(addr, size) == -1) {
        fprintf(stderr, "Error un-mapping the file %s\n", strerror(errno));
        close(fd);
        return;
    }

    close(fd);
}

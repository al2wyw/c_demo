//
// Created by 李扬 on 2026/6/12.
//
/**
 * 动态库搜索路径：
 *    程序指定的库路径‌：首先，程序可能包含了特定的库路径信息。通过编译器选项（-rpath）设置。
 *    ‌环境变量‌：其次，检查LD_LIBRARY_PATH环境变量。
 *    ‌配置文件‌：然后，检查/etc/ld.so.conf和/etc/ld.so.conf.d/*.conf中的路径。
 *    ‌标准系统目录‌：最后，在标准系统目录中搜索，如/lib, /usr/lib, /lib64, /usr/lib64等。
 *
 *  实现原理：
 *    动态链接器 ld-linux.so 在解析符号时，会优先查找 LD_PRELOAD 指定的 .so，
 *    因此本文件中的 malloc 会先于 libc 的 malloc 被找到从而完成拦截。
 *    在 init_real_malloc 内部用 dlsym(RTLD_NEXT, "malloc") 拿到链路中"下一个"
 *    同名符号（即 libc 中真正的 malloc）。
 *
 *    gcc -shared -fPIC -O2 -o libfin.so func_intercept.c -ldl
 *    LD_PRELOAD=./libfin.so ./program
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <execinfo.h>

typedef void *(*malloc_fn_t)(size_t);

static malloc_fn_t real_malloc = NULL;

// 重入保护标志：避免 backtrace/写日志过程中调用malloc再次进入 hook 造成无限递归
static int in_hook = 0;

static void init_real_malloc(void) {
    real_malloc = (malloc_fn_t) dlsym(RTLD_NEXT, "malloc");
    if (real_malloc == NULL) {
        printf("dlsym(RTLD_NEXT, \"malloc\") failed\n");
        exit(1);
    }
}

void *malloc(size_t size) {
    if (real_malloc == NULL) {
        init_real_malloc();
    }

    void *ptr = real_malloc(size);

    if (in_hook) {
        return ptr;
    }
    in_hook = 1;

    // printf 可能会调用 malloc，造成无限递归，使用write来避免 或 in_hook 标识
    // printf("malloc(%zu) = %p\n", size, ptr);
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "malloc(%zu) = %p\n", size, ptr);
    write(STDOUT_FILENO, buf, len);

    // 打印调用栈：
    // 1) backtrace 仅写入用户提供的指针数组，不调用 malloc；
    // 2) backtrace_symbols_fd 直接把符号化结果写到 fd，不会像 backtrace_symbols 那样 malloc 返回字符串数组。
    void *frames[32];
    int nframes = backtrace(frames, 32);
    // 跳过第 0 帧 从第 1 帧开始打印
    if (nframes > 1) {
        const char *hdr = "  --- backtrace ---\n";
        write(STDOUT_FILENO, hdr, 20);
        backtrace_symbols_fd(frames + 1, nframes - 1, STDOUT_FILENO);
        write(STDOUT_FILENO, hdr, 20);
    }

    in_hook = 0;
    return ptr;
}
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

typedef void *(*malloc_fn_t)(size_t);

static malloc_fn_t real_malloc = NULL;

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

    // printf 有可能会调用 malloc，造成无限递归，使用write来避免
    // printf("malloc(%zu) = %p\n", size, ptr);
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "malloc(%zu) = %p\n", size, ptr);
    write(1, buf, len);
    return ptr;
}
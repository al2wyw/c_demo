//
// Created by 李扬 on 2025/6/26.
//

#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>

#define align_size_up_(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
#define align_size_down_(size, alignment) ((size) & ~((alignment) - 1))
#define is_size_aligned_(size, alignment) ((size) == (align_size_up_(size, alignment)))

//函数指针
typedef int (*p_fun)();

// new函数 的汇编代码
char code[] = {
    0x55,                                       //pushq $rbp
    0x48, 0x89, 0xe5,                           //movq %rsp, %rbp
    0xb8, 0x0b, 0x00, 0x00, 0x00,               //movl $0xb, %eax
    0x5d,                                       //popq %rbp
    0xc3                                        //retq
};

int new() {
    return 11;
}

int template_new() {
    //创建一块可读可写可执行的内存区域
    void *temp = mmap (
        NULL, //映射区的开始地址，设置为NULL或0表示由系统决定
        // sizeof(code), //也可以
        getpagesize(), //申请的内存大小按内存页对齐，这里直接调用函数获取内存页大小
        PROT_READ | PROT_WRITE | PROT_EXEC, //映射内存区的权限，可读可写可执行
        MAP_ANONYMOUS | MAP_PRIVATE, //映射对象的类型
        -1, //fd
        0 //offset
    );

    printf("temp addr %p\n", temp);

    memcpy(temp, code, sizeof(code));
    p_fun fun=temp;
    return fun();
}

int template_malloc() {
    size_t size = getpagesize();
    void *temp = malloc(size * 2);//不是任意地址都可以，必须要有PROT_EXEC
    printf("temp addr %p\n", temp);
    temp = (void *)align_size_up_((intptr_t)temp, size);
    printf("temp addr %p\n", temp);
    int err = mprotect(temp, size, PROT_READ | PROT_WRITE | PROT_EXEC);//必须pagesize对齐
    if (err != 0) {
        printf("mprotect err %s\n", strerror(errno));
        return 0;
    }
    memcpy(temp, code, sizeof(code));
    p_fun fun=temp;
    return fun();
}

int template_stk() {
    size_t size = getpagesize();
    char arr[size];
    void* temp = &arr;
    printf("temp addr %p\n", temp);
    temp = (void *)align_size_up_((intptr_t)temp, size);
    printf("temp addr %p\n", temp);
    int err = mprotect(temp, size, PROT_READ | PROT_WRITE | PROT_EXEC);//必须pagesize对齐
    if (err != 0) {
        printf("mprotect err %s\n", strerror(errno));
        return 0;
    }
    memcpy(temp, code, sizeof(code));
    p_fun fun=temp;
    //通过函数指针调用
    return fun();
}

// c++通过dlsym可以实现热替换，但是需要在代码里面显式调用dlsym
void main() {
    int obj = new();
    int obj2 = template_new();
    int obj3 = template_malloc();
    int obj4 = template_stk();
    printf("%d, %d, %d, %d\n", obj, obj2, obj3, obj4);
}
//
// Created by 李扬 on 2025/6/26.
//

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>

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
    //void *temp = malloc(getpagesize());//不是任意地址都可以，必须要有PROT_EXEC

    memcpy(temp, code, sizeof(code));
    p_fun fun=code;
    return fun();
}
// c++通过dlsym可以实现热替换，但是需要在代码里面显式调用dlsym
void main() {
    int obj = new();
    int obj2 = template_new();
    printf("%d, %d\n", obj, obj2);
}
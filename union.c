//
// Created by 李扬 on 2025/8/12.
//

#include<stdio.h>

// union 固定首地址 和 union 按字段最大内存需求开辟一段内存空间，即union就是一段所有字段共用的内存空间
union test {
    int i;
    long l;
};


int main() {
    union test v;
    v.i = 10;
    printf("v.i is %d\n",v.i);
    v.l = 0x100000000; //低32位为int i
    printf("now v.l is %ld! the address is %p\n",v.l, &v.l);
    printf("now v.i is %d! the address is %p\n",v.i, &v.i);
}
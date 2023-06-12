//
// Created by 李扬 on 2023/6/12.
//
/*
 * 开启优化-O后非常随意，直接把调用者保护寄存器的值保存到其他的寄存器里而不是内存
 * 前6个参数 rdi、rsi、rdx、rcx、r8、r9
 * |----------------|
 * | args           | 6个以后的参数，每个参数占64bit，无论大小
 * |----------------| <- rbp + 16 (64bit) / previous rsp (通过rsp设置6个以后的参数)
 * | ret address    | 由call指令完成
 * |----------------|
 * | previous rbp   | 由callee完成
 * |----------------| <- rbp
 * | saved regs     |
 * |----------------|
 * | vars           |
 * |----------------|
 * | others         |
 * |----------------| <- rsp
 */
# include <stdio.h>

int callee(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8)
{
    int loc1 = arg7 + arg8;
    arg1 = arg1 + 16;
    return loc1 + arg1;
}

int caller(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8)
{
    int loc1 = arg1 + 16;
    int loc2 = arg2 + 16;
    int loc3 = arg3 + 16;
    int loc4 = arg4 + 16;
    int loc5 = arg5 + 16;
    int loc6 = arg6 + 16;
    int loc7 = arg7 + 16;
    int loc8 = arg8 + 16;
    //return callee(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    return arg1 + callee(loc1, loc2, loc3, loc4, loc5, loc6, loc7, loc8);
}

int main(void)
{
    int loc1 = 1;
    int loc2 = 2;
    int loc3 = 3;
    int loc4 = 4;
    int loc5 = 5;
    int loc6 = 6;
    int loc7 = 7;
    int loc8 = 8;
    int ret = caller(loc1, loc2, loc3, loc4, loc5, loc6, loc7, loc8);

    printf("%d\n", ret);
    return 0;
}
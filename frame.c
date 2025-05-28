//
// Created by 李扬 on 2023/6/12.
//
/*
 * 寻址: imm(base,index,scale) -> imm + base + index * scale
 *  $0xa(%rsi) -> %rsi + 0xa
 *  $0xa(%rdi,%rsi,$0x2) -> %rdi + %rsi * 0x2 + 0xa
 * 数据传输:
 * mov: mov S D -> D = S (S可以为imm、reg、address, D可以为reg、address, S和D不能同时是address, 所以只有5种类型)
 * lea: lea S D -> D = S (S为address,D为reg,把address的算术结果赋给reg,不做寻址)
 * movabsq $0x0011223344556677, %rax        %rax=0011223344556677
 * movb $-1，%al                            %rax=00112233445566FF
 * movw $-1，%ax                            %rax=001122334455FFFF
 * movl $-1, %eax                           %rax=00000000FFFFFFFF  !!!
 * movg $-1, %rax                           %rax=FFFFFFFFFFFFFFFF
 * 位逻辑运算:
 * xor (xor %rdi %rdi用来置0)
 * and/test (test %rdi %rdi用来检查0、正数、负数)
 * or
 * not
 * 算术运算:
 * sub/comp: sub S D -> D = D - S (S和D的规则和mov相同，当操作数是address时必须先从内存读取数据)
 * add
 * mul
 * 条件码访问:
 * set: sete(setz) D -> D = 1 when comp is equal else D = 0(movb ZF D)(把内存上的单字节或寄存器的高/低8bit置为0或1)
 * jmp: jmpg label -> jump to label when comp is greater
 * cmov: cmovg S D -> D = S when comp is greater
 *
 * 开启优化-O后非常随意，直接把调用者保护寄存器的值保存到其他的寄存器里而不是内存
 * 位数操作 rax 64 eax 32 ax(ah 8 al 8) 16 / movq 64 movl 32 movw 16 movb 8
 * eflags的低8bit(7 -> 0): sf zf 0 af 0 pf 1 cf
 * ZF零标识: 算术或位逻辑运算结果为0,则ZF值为1,否则为0(test和comp只影响标志位)
 * SF符号标识: 运算结果有符号整数的符号位,0表示正数,1表示负数
 * 前6个参数 rdi、rsi、rdx、rcx、r8、r9
 * 函数返回 rax
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
 *
 *        高地址
 * +------------------+
 * |     环境变量区     | ← 环境变量
 * +------------------+
 * |     命令行参数     | ← 命令行参数
 * +------------------+
 * |      stack       | ← 函数调用，局部变量(小)
 * +------------------+
 * |       ↓↓↓        | ← 栈向下增长
 * +------------------+
 * |      共享库       | ← 动态加载的库
 * +------------------+
 * |       ↑↑↑        | ← 堆向上增长
 * +------------------+
 * |       heap       | ← 动态分配内存(大)
 * +------------------+
 * |    未初始化数据段  | ← 未初始化的全局变量
 * |     (BSS段)      |
 * +------------------+
 * |    已初始化数据段  | ← 已初始化的全局变量
 * |     (Data段)     |
 * +------------------+
 * |      代码段       | ← 程序的指令代码
 * +------------------+
 *        低地址
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
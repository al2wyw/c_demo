#include <stdio.h>

int branch(int);

/*
int main(int argc, char *argv[]) {
    int i = branch(argc);//prevent from DCE
    i++;
    printf("%d", i);
}
 */
int main() {
    int i = branch(9);
    i++;
    printf("%d", i);
}

// 通过CFG控制流图和值范围分析来判断恒假、恒真的条件
int branch(int v){
    if (v > 16) {
        printf("what is %d", v);
    } else if (v > 32) { //remove always false branch
        printf("what is %d", v);
    } else if (v > 64) { //remove always false branch
        printf("what is %d", v);
    }

    const int h = 10;
    if (h + 1 < 64) { //remove condition
        printf("what is %d", v);
    }
    return 0;
}

/*
* gcc branch.c -g -O3 -o branch.o -c
* objdump -dS branch.o
* -O3 优化后的main函数:
* int main() {printf("what is %d", 9);printf("%d", 1);}
branch.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <branch>:
    i++;
    printf("%d", i);
}

int branch(int v){
    if (v > 16) {
   0:   83 ff 10                cmp    $0x10,%edi
int branch(int v){
   3:   53                      push   %rbx
   4:   89 fb                   mov    %edi,%ebx
    if (v > 16) {
   6:   7f 18                   jg     20 <branch+0x20>
        printf("what is %d", v);
    }

    const int h = 10;
    if (h + 1 < 20) {
        printf("what is %d", v);
   8:   89 de                   mov    %ebx,%esi
   a:   bf 00 00 00 00          mov    $0x0,%edi
   f:   31 c0                   xor    %eax,%eax
  11:   e8 00 00 00 00          callq  16 <branch+0x16>
    }
    return 0;
}
  16:   31 c0                   xor    %eax,%eax
  18:   5b                      pop    %rbx
  19:   c3                      retq
  1a:   66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)
        printf("what is %d", v);
  20:   89 fe                   mov    %edi,%esi
  22:   31 c0                   xor    %eax,%eax
  24:   bf 00 00 00 00          mov    $0x0,%edi
  29:   e8 00 00 00 00          callq  2e <branch+0x2e>
  2e:   eb d8                   jmp    8 <branch+0x8>

Disassembly of section .text.startup:

0000000000000000 <main>:
    if (v > 16) {
   0:   48 83 ec 08             sub    $0x8,%rsp
int branch(int v){
   4:   be 09 00 00 00          mov    $0x9,%esi
        printf("what is %d", v);
   9:   bf 00 00 00 00          mov    $0x0,%edi
   e:   31 c0                   xor    %eax,%eax
  10:   e8 00 00 00 00          callq  15 <main+0x15>
  15:   be 01 00 00 00          mov    $0x1,%esi
}
  1a:   bf 00 00 00 00          mov    $0x0,%edi
  1f:   31 c0                   xor    %eax,%eax
        printf("what is %d", v);
  21:   48 83 c4 08             add    $0x8,%rsp
  25:   e9 00 00 00 00          jmpq   2a <main+0x2a>
 */



volatile:	file format mach-o 64-bit x86-64


Disassembly of section __TEXT,__text:

0000000100003f20 <_test>:
100003f20: 55                          	pushq	%rbp
100003f21: 48 89 e5                    	movq	%rsp, %rbp
100003f24: 8b 35 ee 40 00 00           	movl	16622(%rip), %esi  # 100008018 <_val>
100003f2a: 85 ff                       	testl	%edi, %edi
100003f2c: 7e 0c                       	jle	0x100003f3a <_test+0x1a>
100003f2e: 8d 04 bf                    	leal	(%rdi,%rdi,4), %eax
100003f31: 8d 34 46                    	leal	(%rsi,%rax,2), %esi
100003f34: 89 35 de 40 00 00           	movl	%esi, 16606(%rip)  # 100008018 <_val>
100003f3a: 48 8d 3d 57 00 00 00        	leaq	87(%rip), %rdi  # 100003f98 <dyld_stub_binder+0x100003f98>
100003f41: 31 c0                       	xorl	%eax, %eax
100003f43: e8 26 00 00 00              	callq	0x100003f6e <dyld_stub_binder+0x100003f6e>
100003f48: 5d                          	popq	%rbp
100003f49: c3                          	retq
100003f4a: 66 0f 1f 44 00 00           	nopw	(%rax,%rax)

0000000100003f50 <_main>:
100003f50: 55                          	pushq	%rbp
100003f51: 48 89 e5                    	movq	%rsp, %rbp
100003f54: 48 8b 7e 08                 	movq	8(%rsi), %rdi
100003f58: e8 0b 00 00 00              	callq	0x100003f68 <dyld_stub_binder+0x100003f68>
100003f5d: 89 c7                       	movl	%eax, %edi
100003f5f: e8 bc ff ff ff              	callq	0x100003f20 <_test>
100003f64: 31 c0                       	xorl	%eax, %eax
100003f66: 5d                          	popq	%rbp
100003f67: c3                          	retq

Disassembly of section __TEXT,__stubs:

0000000100003f68 <__stubs>:
100003f68: ff 25 92 40 00 00           	jmpq	*16530(%rip)  # 100008000 <dyld_stub_binder+0x100008000>
100003f6e: ff 25 94 40 00 00           	jmpq	*16532(%rip)  # 100008008 <dyld_stub_binder+0x100008008>

Disassembly of section __TEXT,__stub_helper:

0000000100003f74 <__stub_helper>:
100003f74: 4c 8d 1d 95 40 00 00        	leaq	16533(%rip), %r11  # 100008010 <__dyld_private>
100003f7b: 41 53                       	pushq	%r11
100003f7d: ff 25 7d 00 00 00           	jmpq	*125(%rip)  # 100004000 <dyld_stub_binder+0x100004000>
100003f83: 90                          	nop
100003f84: 68 00 00 00 00              	pushq	$0
100003f89: e9 e6 ff ff ff              	jmp	0x100003f74 <__stub_helper>
100003f8e: 68 0c 00 00 00              	pushq	$12
100003f93: e9 dc ff ff ff              	jmp	0x100003f74 <__stub_helper>

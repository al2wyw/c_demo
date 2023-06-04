
volatile:	file format mach-o 64-bit x86-64


Disassembly of section __TEXT,__text:

0000000100003ee0 <_test>:
100003ee0: 55                          	pushq	%rbp
100003ee1: 48 89 e5                    	movq	%rsp, %rbp
100003ee4: 48 83 ec 10                 	subq	$16, %rsp
100003ee8: 89 7d fc                    	movl	%edi, -4(%rbp)
100003eeb: c7 45 f8 00 00 00 00        	movl	$0, -8(%rbp)
100003ef2: 8b 45 f8                    	movl	-8(%rbp), %eax
100003ef5: 3b 45 fc                    	cmpl	-4(%rbp), %eax
100003ef8: 0f 8d 1d 00 00 00           	jge	0x100003f1b <_test+0x3b>
100003efe: 8b 05 14 41 00 00           	movl	16660(%rip), %eax  # 100008018 <_val>
100003f04: 83 c0 0a                    	addl	$10, %eax
100003f07: 89 05 0b 41 00 00           	movl	%eax, 16651(%rip)  # 100008018 <_val>
100003f0d: 8b 45 f8                    	movl	-8(%rbp), %eax
100003f10: 83 c0 01                    	addl	$1, %eax
100003f13: 89 45 f8                    	movl	%eax, -8(%rbp)
100003f16: e9 d7 ff ff ff              	jmp	0x100003ef2 <_test+0x12>
100003f1b: 8b 35 f7 40 00 00           	movl	16631(%rip), %esi  # 100008018 <_val>
100003f21: 48 8d 3d 78 00 00 00        	leaq	120(%rip), %rdi  # 100003fa0 <dyld_stub_binder+0x100003fa0>
100003f28: b0 00                       	movb	$0, %al
100003f2a: e8 47 00 00 00              	callq	0x100003f76 <dyld_stub_binder+0x100003f76>
100003f2f: 48 83 c4 10                 	addq	$16, %rsp
100003f33: 5d                          	popq	%rbp
100003f34: c3                          	retq
100003f35: 66 2e 0f 1f 84 00 00 00 00 00       	nopw	%cs:(%rax,%rax)
100003f3f: 90                          	nop

0000000100003f40 <_main>:
100003f40: 55                          	pushq	%rbp
100003f41: 48 89 e5                    	movq	%rsp, %rbp
100003f44: 48 83 ec 20                 	subq	$32, %rsp
100003f48: 89 7d fc                    	movl	%edi, -4(%rbp)
100003f4b: 48 89 75 f0                 	movq	%rsi, -16(%rbp)
100003f4f: 48 8b 45 f0                 	movq	-16(%rbp), %rax
100003f53: 48 8b 78 08                 	movq	8(%rax), %rdi
100003f57: e8 14 00 00 00              	callq	0x100003f70 <dyld_stub_binder+0x100003f70>
100003f5c: 89 45 ec                    	movl	%eax, -20(%rbp)
100003f5f: 8b 7d ec                    	movl	-20(%rbp), %edi
100003f62: e8 79 ff ff ff              	callq	0x100003ee0 <_test>
100003f67: 31 c0                       	xorl	%eax, %eax
100003f69: 48 83 c4 20                 	addq	$32, %rsp
100003f6d: 5d                          	popq	%rbp
100003f6e: c3                          	retq

Disassembly of section __TEXT,__stubs:

0000000100003f70 <__stubs>:
100003f70: ff 25 8a 40 00 00           	jmpq	*16522(%rip)  # 100008000 <dyld_stub_binder+0x100008000>
100003f76: ff 25 8c 40 00 00           	jmpq	*16524(%rip)  # 100008008 <dyld_stub_binder+0x100008008>

Disassembly of section __TEXT,__stub_helper:

0000000100003f7c <__stub_helper>:
100003f7c: 4c 8d 1d 8d 40 00 00        	leaq	16525(%rip), %r11  # 100008010 <__dyld_private>
100003f83: 41 53                       	pushq	%r11
100003f85: ff 25 75 00 00 00           	jmpq	*117(%rip)  # 100004000 <dyld_stub_binder+0x100004000>
100003f8b: 90                          	nop
100003f8c: 68 00 00 00 00              	pushq	$0
100003f91: e9 e6 ff ff ff              	jmp	0x100003f7c <__stub_helper>
100003f96: 68 0c 00 00 00              	pushq	$12
100003f9b: e9 dc ff ff ff              	jmp	0x100003f7c <__stub_helper>

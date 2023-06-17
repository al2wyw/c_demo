
frame:	file format mach-o 64-bit x86-64


Disassembly of section __TEXT,__text:

0000000100003df0 <_callee>:
100003df0: 55                          	pushq	%rbp
100003df1: 48 89 e5                    	movq	%rsp, %rbp
100003df4: 8b 45 18                    	movl	24(%rbp), %eax
100003df7: 44 8b 55 10                 	movl	16(%rbp), %r10d
100003dfb: 89 7d fc                    	movl	%edi, -4(%rbp)
100003dfe: 89 75 f8                    	movl	%esi, -8(%rbp)
100003e01: 89 55 f4                    	movl	%edx, -12(%rbp)
100003e04: 89 4d f0                    	movl	%ecx, -16(%rbp)
100003e07: 44 89 45 ec                 	movl	%r8d, -20(%rbp)
100003e0b: 44 89 4d e8                 	movl	%r9d, -24(%rbp)
100003e0f: 8b 4d 10                    	movl	16(%rbp), %ecx
100003e12: 03 4d 18                    	addl	24(%rbp), %ecx
100003e15: 89 4d e4                    	movl	%ecx, -28(%rbp)
100003e18: 8b 4d fc                    	movl	-4(%rbp), %ecx
100003e1b: 83 c1 10                    	addl	$16, %ecx
100003e1e: 89 4d fc                    	movl	%ecx, -4(%rbp)
100003e21: 8b 4d e4                    	movl	-28(%rbp), %ecx
100003e24: 03 4d fc                    	addl	-4(%rbp), %ecx
100003e27: 89 45 e0                    	movl	%eax, -32(%rbp)
100003e2a: 89 c8                       	movl	%ecx, %eax
100003e2c: 5d                          	popq	%rbp
100003e2d: c3                          	retq
100003e2e: 66 90                       	nop

0000000100003e30 <_caller>:
100003e30: 55                          	pushq	%rbp
100003e31: 48 89 e5                    	movq	%rsp, %rbp
100003e34: 41 56                       	pushq	%r14
100003e36: 53                          	pushq	%rbx
100003e37: 48 83 ec 60                 	subq	$96, %rsp
100003e3b: 8b 45 18                    	movl	24(%rbp), %eax
100003e3e: 44 8b 55 10                 	movl	16(%rbp), %r10d
100003e42: 89 7d ec                    	movl	%edi, -20(%rbp)
100003e45: 89 75 e8                    	movl	%esi, -24(%rbp)
100003e48: 89 55 e4                    	movl	%edx, -28(%rbp)
100003e4b: 89 4d e0                    	movl	%ecx, -32(%rbp)
100003e4e: 44 89 45 dc                 	movl	%r8d, -36(%rbp)
100003e52: 44 89 4d d8                 	movl	%r9d, -40(%rbp)
100003e56: 8b 4d ec                    	movl	-20(%rbp), %ecx
100003e59: 83 c1 10                    	addl	$16, %ecx
100003e5c: 89 4d d4                    	movl	%ecx, -44(%rbp)
100003e5f: 8b 4d e8                    	movl	-24(%rbp), %ecx
100003e62: 83 c1 10                    	addl	$16, %ecx
100003e65: 89 4d d0                    	movl	%ecx, -48(%rbp)
100003e68: 8b 4d e4                    	movl	-28(%rbp), %ecx
100003e6b: 83 c1 10                    	addl	$16, %ecx
100003e6e: 89 4d cc                    	movl	%ecx, -52(%rbp)
100003e71: 8b 4d e0                    	movl	-32(%rbp), %ecx
100003e74: 83 c1 10                    	addl	$16, %ecx
100003e77: 89 4d c8                    	movl	%ecx, -56(%rbp)
100003e7a: 8b 4d dc                    	movl	-36(%rbp), %ecx
100003e7d: 83 c1 10                    	addl	$16, %ecx
100003e80: 89 4d c4                    	movl	%ecx, -60(%rbp)
100003e83: 8b 4d d8                    	movl	-40(%rbp), %ecx
100003e86: 83 c1 10                    	addl	$16, %ecx
100003e89: 89 4d c0                    	movl	%ecx, -64(%rbp)
100003e8c: 8b 4d 10                    	movl	16(%rbp), %ecx
100003e8f: 83 c1 10                    	addl	$16, %ecx
100003e92: 89 4d bc                    	movl	%ecx, -68(%rbp)
100003e95: 8b 4d 18                    	movl	24(%rbp), %ecx
100003e98: 83 c1 10                    	addl	$16, %ecx
100003e9b: 89 4d b8                    	movl	%ecx, -72(%rbp)
100003e9e: 8b 4d ec                    	movl	-20(%rbp), %ecx
100003ea1: 8b 7d d4                    	movl	-44(%rbp), %edi
100003ea4: 8b 75 d0                    	movl	-48(%rbp), %esi
100003ea7: 8b 55 cc                    	movl	-52(%rbp), %edx
100003eaa: 44 8b 45 c8                 	movl	-56(%rbp), %r8d
100003eae: 44 8b 4d c4                 	movl	-60(%rbp), %r9d
100003eb2: 44 8b 5d c0                 	movl	-64(%rbp), %r11d
100003eb6: 8b 5d bc                    	movl	-68(%rbp), %ebx
100003eb9: 44 8b 75 b8                 	movl	-72(%rbp), %r14d
100003ebd: 89 4d b4                    	movl	%ecx, -76(%rbp)
100003ec0: 44 89 c1                    	movl	%r8d, %ecx
100003ec3: 45 89 c8                    	movl	%r9d, %r8d
100003ec6: 45 89 d9                    	movl	%r11d, %r9d
100003ec9: 89 1c 24                    	movl	%ebx, (%rsp)
100003ecc: 44 89 74 24 08              	movl	%r14d, 8(%rsp)
100003ed1: 89 45 b0                    	movl	%eax, -80(%rbp)
100003ed4: 44 89 55 ac                 	movl	%r10d, -84(%rbp)
100003ed8: e8 13 ff ff ff              	callq	0x100003df0 <_callee>
100003edd: 8b 4d b4                    	movl	-76(%rbp), %ecx
100003ee0: 01 c1                       	addl	%eax, %ecx
100003ee2: 89 c8                       	movl	%ecx, %eax
100003ee4: 48 83 c4 60                 	addq	$96, %rsp
100003ee8: 5b                          	popq	%rbx
100003ee9: 41 5e                       	popq	%r14
100003eeb: 5d                          	popq	%rbp
100003eec: c3                          	retq
100003eed: 0f 1f 00                    	nopl	(%rax)

0000000100003ef0 <_main>:
100003ef0: 55                          	pushq	%rbp
100003ef1: 48 89 e5                    	movq	%rsp, %rbp
100003ef4: 48 83 ec 40                 	subq	$64, %rsp
100003ef8: c7 45 fc 00 00 00 00        	movl	$0, -4(%rbp)
100003eff: c7 45 f8 01 00 00 00        	movl	$1, -8(%rbp)
100003f06: c7 45 f4 02 00 00 00        	movl	$2, -12(%rbp)
100003f0d: c7 45 f0 03 00 00 00        	movl	$3, -16(%rbp)
100003f14: c7 45 ec 04 00 00 00        	movl	$4, -20(%rbp)
100003f1b: c7 45 e8 05 00 00 00        	movl	$5, -24(%rbp)
100003f22: c7 45 e4 06 00 00 00        	movl	$6, -28(%rbp)
100003f29: c7 45 e0 07 00 00 00        	movl	$7, -32(%rbp)
100003f30: c7 45 dc 08 00 00 00        	movl	$8, -36(%rbp)
100003f37: 8b 7d f8                    	movl	-8(%rbp), %edi
100003f3a: 8b 75 f4                    	movl	-12(%rbp), %esi
100003f3d: 8b 55 f0                    	movl	-16(%rbp), %edx
100003f40: 8b 4d ec                    	movl	-20(%rbp), %ecx
100003f43: 44 8b 45 e8                 	movl	-24(%rbp), %r8d
100003f47: 44 8b 4d e4                 	movl	-28(%rbp), %r9d
100003f4b: 8b 45 e0                    	movl	-32(%rbp), %eax
100003f4e: 44 8b 55 dc                 	movl	-36(%rbp), %r10d
100003f52: 89 04 24                    	movl	%eax, (%rsp)
100003f55: 44 89 54 24 08              	movl	%r10d, 8(%rsp)
100003f5a: e8 d1 fe ff ff              	callq	0x100003e30 <_caller>
100003f5f: 89 45 d8                    	movl	%eax, -40(%rbp)
100003f62: 8b 75 d8                    	movl	-40(%rbp), %esi
100003f65: 48 8d 3d 36 00 00 00        	leaq	54(%rip), %rdi  # 100003fa2 <dyld_stub_binder+0x100003fa2>
100003f6c: b0 00                       	movb	$0, %al
100003f6e: e8 0d 00 00 00              	callq	0x100003f80 <dyld_stub_binder+0x100003f80>
100003f73: 31 c9                       	xorl	%ecx, %ecx
100003f75: 89 45 d4                    	movl	%eax, -44(%rbp)
100003f78: 89 c8                       	movl	%ecx, %eax
100003f7a: 48 83 c4 40                 	addq	$64, %rsp
100003f7e: 5d                          	popq	%rbp
100003f7f: c3                          	retq

Disassembly of section __TEXT,__stubs:

0000000100003f80 <__stubs>:
100003f80: ff 25 7a 40 00 00           	jmpq	*16506(%rip)  # 100008000 <dyld_stub_binder+0x100008000>

Disassembly of section __TEXT,__stub_helper:

0000000100003f88 <__stub_helper>:
100003f88: 4c 8d 1d 79 40 00 00        	leaq	16505(%rip), %r11  # 100008008 <__dyld_private>
100003f8f: 41 53                       	pushq	%r11
100003f91: ff 25 69 00 00 00           	jmpq	*105(%rip)  # 100004000 <dyld_stub_binder+0x100004000>
100003f97: 90                          	nop
100003f98: 68 00 00 00 00              	pushq	$0
100003f9d: e9 e6 ff ff ff              	jmp	0x100003f88 <__stub_helper>

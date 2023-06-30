	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 13, 0	sdk_version 13, 3
	.globl	_mul                            ## -- Begin function mul
	.p2align	4, 0x90
_mul:                                   ## @mul
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	_mystr(%rip), %rsi
	leaq	L_.str.3(%rip), %rdi
	movb	$0, %al
	callq	_printf
	movslq  _val(%rip), %rdi
    call    _print
	movq	-8(%rbp), %rcx
	imulq	-16(%rbp), %rcx
	shlq	$0, %rcx
	movslq	_val(%rip), %rdx
	imulq	%rdx, %rcx
	movslq	_static_val(%rip), %rdx
	imulq	%rdx, %rcx
	movl	%eax, -20(%rbp)                 ## 4-byte Spill
	movq	%rcx, %rax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__const
	.globl	_const_val                      ## @const_val
	.p2align	2
_const_val:
	.long	1                               ## 0x1

	.section	__DATA,__data
	.globl	_val                            ## @val
	.p2align	2
_val:
	.long	3                               ## 0x3

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"charstrings"

	.section	__DATA,__data
	.globl	_mystr                          ## @mystr
	.p2align	3
_mystr:
	.quad	L_.str

	.section	__TEXT,__cstring,cstring_literals
L_.str.1:                               ## @.str.1
	.asciz	"node"

	.section	__DATA,__data
	.globl	_tarnode                        ## @tarnode
	.p2align	3
_tarnode:
	.long	544                             ## 0x220
	.space	4
	.quad	L_.str.1

	.section	__TEXT,__cstring,cstring_literals
L_.str.2:                               ## @.str.2
	.asciz	"myvalue"

	.section	__DATA,__data
	.globl	_tar                            ## @tar
	.p2align	3
_tar:
	.long	123                             ## 0x7b
	.space	4
	.quad	L_.str.2
	.quad	_tarnode

	.section	__TEXT,__cstring,cstring_literals
L_.str.3:                               ## @.str.3
	.asciz	"%s\n"

	.section	__DATA,__data
	.p2align	2                               ## @static_val
_static_val:
	.long	2                               ## 0x2

.subsections_via_symbols

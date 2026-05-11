.L3:
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    %r12d, %ebx
        addl    $1, -20(%rbp)
.L2:
        movl    -20(%rbp), %eax
        cmpl    -36(%rbp), %eax
        jl      .L3

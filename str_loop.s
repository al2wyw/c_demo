.L3:
        movl    %ebp, %eax
        sall    $4, %eax
        addl    %eax, %ebx
        addl    $1, %edx
        cmpl    %r12d, %edx
        jne     .L3 

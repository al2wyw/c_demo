//
// Created by root on 6/17/23.
//

#include <stdio.h>

#ifdef CC_INTR
int cc_intr = 1;
#else
int asm_intr = 2;
#endif


int main(void) {
#ifdef CC_INTR
    printf("%d\n", cc_intr);
#else
    printf("%d\n", asm_intr);
#endif
}
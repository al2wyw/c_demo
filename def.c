//
// Created by root on 6/17/23.
//

#include <stdio.h>

#ifdef CC_INTR
int cc_intr = 1;
#else
int asm_intr = 2;
#endif

#define contract(name1, name2) name1##name2
#define str(name) #name
#define contract_str(name1, name2) name1 name2

int main(void) {
#ifdef CC_INTR
    printf("%d\n", cc_intr);
#else
    printf("%d\n", asm_intr);
#endif

    contract(print, f)("%s\n", str(contract(cc_intr)));
    contract(print, f)("%s\n", str(contract(asm_intr)));
    printf("%s\n", contract_str("cc_intr", "#asm_intr"));
}
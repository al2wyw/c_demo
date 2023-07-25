#include <sys/types.h>
#include <stdio.h>

typedef struct {
    int i;
    char* value;
} node, mynode;

typedef struct {
    int i;
    char* value;
    mynode* node;
} my, mystruct;

const int const_val = 1;
static int static_val = 2;
int val = 3;
char* mystr = "charstrings";
mynode tarnode = {544, "node"};
mystruct tar = {123, "myvalue", &tarnode};

int64_t mul(int64_t a, int64_t b) {
    if (val > 1) {
        printf("%s\n", mystr);
    }
    return a * b * const_val * val * static_val;
}
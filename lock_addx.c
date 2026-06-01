//
// Created by root on 6/4/23.
//

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

//order access
// loadload loadstore -> acquire
// storestore -> release
// storeload -> fence
void acquire() {
    volatile int* local_dummy;

    __asm__ volatile ("movq 0(%%rsp), %0" : "=r" (local_dummy) : : "memory");
}

void release() {
    // Avoid hitting the same cache-line from
    // different threads.
    volatile int local_dummy = 0;
}

void fence() {
    // always use locked addl since mfence is sometimes expensive
    __asm__ volatile ("lock; addl $0,0(%%rsp)" : : : "cc", "memory");
}
/* gcc -O3 -> objdump -S
0000000000400550 <acquire>: // 读内存
  400550:       48 8b 04 24             mov    (%rsp),%rax
  400554:       c3                      ret

0000000000400560 <release>: // 写内存
  400560:       c7 44 24 fc 00 00 00    movl   $0x0,-0x4(%rsp)
  400567:       00
  400568:       c3                      ret

0000000000400570 <fence>: // lock + 读写内存
  400570:       f0 83 04 24 00          lock addl $0x0,(%rsp)
  400575:       c3                      ret
 */

intptr_t load_acquire(volatile intptr_t*   p) { return *p; }
void release_store(volatile intptr_t* p, intptr_t v) { *p = v; }
void store_fence(intptr_t* p, intptr_t v) {
    __asm__ __volatile__ ("xchgq (%2), %0"
                          : "=r" (v)
                          : "0" (v), "r" (p)
                          : "memory");
}
void release_store_fence(volatile intptr_t* p, intptr_t v) {
    __asm__ __volatile__ ("xchgq (%2), %0"
                          : "=r" (v)
                          : "0" (v), "r" (p)
                          : "memory");
}
//order access

#define THREAD_SIZE     10

int inc(int *value, int add) {
    int old;
    __asm__ volatile (
    "lock; xaddl %2, %1;"
    : "=a" (old)
    : "m" (*value), "a" (add)
    : "cc", "memory"
    );
    return old;
}
// cc: code condition register

#define LOCK_IF_MP(mp) "cmp $0, " #mp "; je 1f; lock; 1: "

int cmpxchg(int exchange_value,volatile int *dest,int compare_value){
    int mp = 1;
    __asm__ volatile (LOCK_IF_MP(%4) "cmpxchgl %1,(%3)"
    : "=a" (exchange_value)
    : "r" (exchange_value), "a" (compare_value), "r" (dest), "r" (mp)
    : "cc", "memory");
    return exchange_value;
}
/*
int cmpxchg(int exchange_value,volatile int *dest,int compare_value){
    int mp = 1;
    __asm__ volatile (LOCK_IF_MP(%4) "cmpxchgl %1,%3"
    : "=a" (exchange_value)
    : "r" (exchange_value), "a" (compare_value), "m" (*dest), "r" (mp)
    : "cc", "memory");
    return exchange_value;
}
*/
// "m" with pointer variable is a little bit tricky

int add() {
    int result= 0;
    int input = 1;
    __asm__ __volatile__ ("addl %1,%0"::"m"(result), "r"(input): "memory"); //may be invalid use of "m"
    // __asm__ __volatile__ ("addl %2,%0":"=r" (result):"0"(result), "r"(input):"memory");
    // 0,1,2,3.... use for input-output variable

    printf("Hello, World! %d\n", result);
    return result;
}

//callback
void *func(void *arg) {
    int *pcount = (int *) arg;
    int i = 0;
    while (i++ < 10000) {
        inc(pcount, 1);

        usleep(1);
    }
}


int main() {
    add();
    pthread_t th_id[THREAD_SIZE] = {0};

    int i = 0;
    int count = 0;

    for (i = 0; i < THREAD_SIZE; i++) {
        int ret = pthread_create(&th_id[i], NULL, func, &count);
        if (ret) {
            break;
        }
    }
    for (i = 0; i < 100; i++) {
        printf("count --> %d\n", count);

        sleep(2);
    }
}
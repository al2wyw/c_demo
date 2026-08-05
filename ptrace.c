//
// Created by 李扬 on 2026/8/5.
//


#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    pid_t child = fork();
    if (child == 0) {
        // 子进程：让父进程可以跟踪
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execl("/bin/ls", "ls", NULL);
    } else {
        int status;
        waitpid(child, &status, 0);  // 等待子进程 exec 后停下

        for (int i = 0; i < 10 && WIFSTOPPED(status); i++) {
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, child, 0, &regs);
            printf("Step %d: RIP=0x%llx\n", i, regs.rip);

            ptrace(PTRACE_SINGLESTEP, child, 0, 0);
            waitpid(child, &status, 0);
        }
        ptrace(PTRACE_CONT, child, 0, 0);
    }
    return 0;
}
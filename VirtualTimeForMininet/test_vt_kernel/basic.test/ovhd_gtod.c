#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// #include "syscall.wrap/syscall_wrapper.h"
#include <syscall.h>
#include "util.h"

long int without_virtual_time()
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    struct timeval tmp;

    long ret;
    ret = gettimeofday(&prev, NULL);
    chk_sys_call_ret(ret, "gettimeofday");

    long int i;
    for (i = 0; i < NR_ROUND; ++i)
    {
        ret = gettimeofday(&tmp, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
    }
    ret = gettimeofday(&next, NULL);
    chk_sys_call_ret(ret, "gettimeofday");
    ret = timeval_substract(&diff, &next, &prev);
    printf("Elapsed %ld seconds, %ld useconds\n", diff.tv_sec, diff.tv_usec);

    long int usec = timeval_to_usec(diff);
    printf("Elapsed %ld useconds without virtual time\n", usec);
    return usec;
}

long int with_virtual_time()
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    struct timeval tmp;

    long ret;
    int status;
    int pid = fork();

    ret = gettimeofday(&prev, NULL);
    chk_sys_call_ret(ret, "gettimeofday");

    if (pid == -1)
    {
        printf("\n[error] clone_time fails with error: %s\n", strerror(errno));

    } else if (pid == 0) {

        ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, 4);
        chk_sys_call_ret(ret, "virtualtimeunshare");

        long int i;
        for (i = 0; i < NR_ROUND; ++i)
        {
            ret = gettimeofday(&tmp, NULL);
            chk_sys_call_ret(ret, "gettimeofday");
        }
        exit(EXIT_SUCCESS);
        
    } else {

        pid = wait(&status);
        // if (status == -1)
        // {
        //     perror("wait error");
        //     return -1;
        // }
        // if (WIFEXITED(status) != 0) {
        //     printf("Child process ended normally; status = %d\n", WEXITSTATUS(status));
        // }
        ret = gettimeofday(&next, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        ret = timeval_substract(&diff, &next, &prev);
        printf("Elapsed %ld seconds, %ld useconds\n", diff.tv_sec, diff.tv_usec);

        long int usec = timeval_to_usec(diff);
        printf("Elapsed %ld useconds with virtual time\n", usec);
        return usec;
    }
}

int main(int argc, char const *argv[])
{
    long int noVT = without_virtual_time();
    long int VT = with_virtual_time();
    printf("Tot Overhead = %d usec\n", VT - noVT);
    printf("Avg Overhead = %f usec\n", ((float)(VT - noVT)) / NR_ROUND);
    return 0;
}
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "syscall.wrap/syscall_wrapper.h"
#include "util.h"

#undef NR_ROUND
#define NR_ROUND 100

int elapsed[NR_ROUND];
int dilated_elapsed[NR_ROUND];

void fill_elapsed()
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    long ret;
    long int i, j;
    for (i = 0; i < NR_ROUND; ++i)
    {
        ret = gettimeofday(&prev, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        for (j = 0; j < CNT_SLEEP; ++j)
        {
            // do nothing
        }
        ret = gettimeofday(&next, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        ret = timeval_substract(&diff, &next, &prev);
        long int usec = timeval_to_usec(diff);
        elapsed[i] = usec;
    }
}

void fill_dilated_elapsed(int dil)
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    long ret;

    ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, dil);
    chk_sys_call_ret(ret, "virtualtimeunshare");

    long int i, j;
    for (i = 0; i < NR_ROUND; ++i)
    {
        ret = gettimeofday(&prev, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        for (j = 0; j < CNT_SLEEP; ++j)
        {
            // do nothing
        }
        ret = gettimeofday(&next, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        ret = timeval_substract(&diff, &next, &prev);
        long int usec = timeval_to_usec(diff);
        dilated_elapsed[i] = usec;
    }
}

void actual_dilation(int dil)
{
    float q;
    int i;
    int count = 0;
    for (i = 0; i < NR_ROUND; ++i)
    {
        q = (float)elapsed[i] / (float)dilated_elapsed[i];
        if ((q - dil)*(q - dil) > 1)
        {
            printf("[error] round %d: %f\n", i, q);
            ++count;
        } else {
            printf("[good] round %d: %f\n", i, q);
        }
    }
    printf("[summary] %d bad dilations\n", count);
}

const char* program_name;

int main(int argc, char const *argv[])
{
    const char* const short_options = "t:edp";
    const struct option long_options[] = {
        { "tdf", 1, NULL, 't'},
        { "elapsed", 0, NULL, 'e'},
        { "dilated", 0, NULL, 'd'},
        { "print", 0, NULL, 'p'},
        { NULL, 0, NULL, 0}
    };

    int next_option;
    int run_elapsed = 0;
    int run_dilated = 0;
    int print_dil = 0;
    int dilation = 1;

    program_name = argv[0];

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch(next_option)
        {
            case 't':
                dilation = atoi(optarg);
                break;
            case 'e':
                run_elapsed = 1;
                break;
            case 'd':
                // if (dilation == 1) {
                //     printf("[error] run dilated program should feed non 1 TDF\n");
                //     exit(1);
                // }
                run_dilated = 1;
                break;
            case 'p':
                if (run_elapsed == 1 && run_dilated == 1)
                {
                    print_dil = 1;
                }
                break;
            case -1:
                printf("invalid input parameters\n");
                break;
            default:
                printf("abort\n");
                abort();
                break;
        }
    } while(next_option != -1);

    if (run_elapsed)
    {
        fill_elapsed();
    }
    if (run_dilated)
    {
        fill_dilated_elapsed(dilation);
    }
    if (print_dil) {
        actual_dilation(dilation);
    }

    return 0;
}












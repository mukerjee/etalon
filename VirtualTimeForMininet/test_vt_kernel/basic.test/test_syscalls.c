#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "syscall.wrap/syscall_wrapper.h"
#include "util.h"


/*int testTimeClone(int flags, float dil)
{
    printf("\n[info] Begin for timeclone call with dilation %f\n", dil);

    int pid = timeclone(flags | 0x02000000, 0, NULL, dil, NULL);

    if(pid == -1){
        printf("\n[error] clone_time fails with error: %s\n", strerror(errno));
    } else if (pid == 0){

    struct timeval child_now;
    struct timeval child_later;
    long ret = gettimeofday(&child_now, NULL);
    // long ret = getvirtualtimeofday(&child_now, NULL);
    chk_sys_call_ret(ret, "getvirtualtimeofday");
    int i;
    for(i = 0; i < 1000000000; i++) {
        // do nothing
    }
    ret = gettimeofday(&child_later, NULL);
    // ret = getvirtualtimeofday(&child_later, NULL);
    chk_sys_call_ret(ret, "getvirtualtimeofday");

    printf("\n[info] child %d elapsed %lld: %lld\n", getpid(), child_later.tv_sec - child_now.tv_sec, child_later.tv_usec - child_now.tv_usec);
    } else {

    printf("\n[info] I created a child %d with dilation %f\n", pid, dil);
    struct timeval parent_now;
    struct timeval parent_later;
    long ret = gettimeofday(&parent_now, NULL);
    // long ret = getvirtualtimeofday(&parent_now, NULL);
    chk_sys_call_ret(ret, "getvirtualtimeofday");

    int i;
    for(i = 0; i < 1000000000; i++) {
        // do nothing
    }
    ret = gettimeofday(&parent_later, NULL);
    // ret = getvirtualtimeofday(&parent_later, NULL);
    chk_sys_call_ret(ret, "getvirtualtimeofday");

    printf("\n[info] parent elapsed %lld: %lld\n", parent_later.tv_sec - parent_now.tv_sec, parent_later.tv_usec - parent_now.tv_usec);
    }
    return 0;
}
*/

int testVirtualTimeUnshare(float dil)
{
    printf("\n[info] Begin for virtualtimeunshare call with dilation %f\n", dil);

    /* Test virtualtimeunshare */
    int pid = fork();

    if (pid == -1)
    {
        printf("\n[error] clone_time fails with error: %s\n", strerror(errno));
    } else if (pid == 0) {
        struct timeval now;
        struct timeval later;

        printf("\nstill good before gettimeofday\n");
        // long ret = getvirtualtimeofday(&now, NULL);
        long ret = gettimeofday(&now, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");
        printf("\nstill good after gettimeofday\n");
        int i;
        for(i = 0; i < 1000000000; i++) {
        // do nothing
        }
        // ret = getvirtualtimeofday(&later, NULL);
        ret = gettimeofday(&later, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        printf("\n[info] before virtualtimeunshare process_%d elapsed %lld: %lld\n", getpid(), later.tv_sec - now.tv_sec, later.tv_usec - now.tv_usec);

        ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, dil);
        chk_sys_call_ret(ret, "virtualtimeunshare");

        // ret = getvirtualtimeofday(&now, NULL);
        ret = gettimeofday(&now, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        for(i = 0; i < 1000000000; i++) {
        // do nothing
        }
        // ret = getvirtualtimeofday(&later, NULL);
        ret = gettimeofday(&later, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        printf("\n[info] after virtualtimeunshare process_%d elapsed %lld: %lld\n", getpid(), later.tv_sec - now.tv_sec, later.tv_usec - now.tv_usec);
    }

    return 0;
}

/*
* Unfortunately, this test cannot guarantee a succuess for Mininet
*/
int testSetTimeDilationFactor(float dil)
{
    printf("\n[info] Begin for settimedilationfactor call with dilation %f\n", dil);

    /* Test virtualtimeunshare */
    int pid = fork(); // if pid == 0, it's in child process

    if (pid == -1)
    {
        printf("\n[error] clone_time fails with error: %s\n", strerror(errno));
    } else if (pid == 0) {
        /* pid == 0; code for child */
        struct timeval now;
        struct timeval later;

        // let process know virtual time
        long ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, dil);

        /* make first time access to virtual past time not zero */
        int i;
        for(i = 0; i < 100000000; i++) {
            // do nothing
        }
        // ret = getvirtualtimeofday(&now, NULL);
        ret = gettimeofday(&now, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        for(i = 0; i < 100000000; i++) {
            // do nothing
        }
        // ret = getvirtualtimeofday(&later, NULL);
        ret = gettimeofday(&later, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        printf("\n[info] before settimedilationfactor process_%d elapsed %lld: %lld\n", getpid(), later.tv_sec - now.tv_sec, later.tv_usec - now.tv_usec);

        /* change TDF */
        int ppid = getppid();
        ret = settimedilationfactor(dil * 4, 0); // pass 0 to change caller's dilation
        chk_sys_call_ret(ret, "settimedilationfactor");

        /* make first time access to virtual past time not zero */
        for(i = 0; i < 100000000; i++) {
            // do nothing
        }

        // ret = getvirtualtimeofday(&now, NULL);
        ret = gettimeofday(&now, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        for(i = 0; i < 100000000; i++) {
            // do nothing
        }
        // ret = getvirtualtimeofday(&later, NULL);
        ret = gettimeofday(&later, NULL);
        // chk_sys_call_ret(ret, "getvirtualtimeofday");
        chk_sys_call_ret(ret, "gettimeofday");

        printf("\n[info] after settimedilationfactor process_%d elapsed %lld: %lld\n", getpid(), later.tv_sec - now.tv_sec, later.tv_usec - now.tv_usec);

    }

    return 0;
}

int main(int argc, char *argv[])
{
    int pid;
    int flags;
    int i, count;
    flags = 0;
    float dil;

    if (argc < 2) {
    printf("\n[error] Need to add some arguments\n");
    return 0;
    }

    sscanf(argv[1], "%f", &dil);
    sscanf(argv[2], "%d", &count);

    // testTimeClone(flags, dil);

    testVirtualTimeUnshare(dil);

    // testSetTimeDilationFactor(dil);
    return 0;
}


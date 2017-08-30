
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

#include "util.h"
#include "syscall.wrap/syscall_wrapper.h"


long int repeat_set_tdf_for_myself()
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    struct timeval tmp;

    long ret;
    int status;
    int pid = fork();
    long int i;


    if (pid == -1)
    {
        printf("\n[error] clone_time fails with error: %s\n", strerror(errno));

    } else if (pid == 0) {

        ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, 1);
        chk_sys_call_ret(ret, "virtualtimeunshare");

        for (i = 0; i < NR_ROUND; ++i)
        {
            ret = settimedilationfactor(1, 0); // change itself
            chk_sys_call_ret(ret, "settimedilationfactor");
        }
        exit(EXIT_SUCCESS);
        
    } else {
    	ret = gettimeofday(&prev, NULL);
    	chk_sys_call_ret(ret, "gettimeofday");

        pid = wait(&status);
        ret = gettimeofday(&next, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        ret = timeval_substract(&diff, &next, &prev);
        printf("Elapsed %ld seconds, %ld useconds\n", diff.tv_sec, diff.tv_usec);

        long int usec = timeval_to_usec(diff);
        printf("Elapsed %ld useconds for change TDF\n", usec);

        return usec;
    }
}

long int repeat_set_tdf_for_my_parent()
{
    struct timeval prev;
    struct timeval next;
    struct timeval diff;
    struct timeval tmp;

    long ret;
    int status;
    int pid = fork();
    long int i;


    if (pid == -1)
    {
        printf("\n[error] fork fails with error: %s\n", strerror(errno));

    } else if (pid == 0) {

    	pid = fork();
    	if (pid == -1)
    	{
    		printf("\n[error] fork fails with error: %s\n", strerror(errno));
    	} else if (pid == 0) {
    		ret = virtualtimeunshare(CLONE_NEWNET|CLONE_NEWNS, 1);
        	chk_sys_call_ret(ret, "virtualtimeunshare");

        	int ppid = getppid();

        	for (i = 0; i < NR_ROUND; ++i)
        	{
        	    ret = settimedilationfactor(1, ppid); // change my parents
        	    chk_sys_call_ret(ret, "settimedilationfactor");
        	}
        	exit(EXIT_SUCCESS);
    	} else {
    		pid = wait(&status);
    		exit(EXIT_SUCCESS);
    	}

    } else {
    	ret = gettimeofday(&prev, NULL);
    	chk_sys_call_ret(ret, "gettimeofday");

        pid = wait(&status);
        ret = gettimeofday(&next, NULL);
        chk_sys_call_ret(ret, "gettimeofday");
        ret = timeval_substract(&diff, &next, &prev);
        printf("Elapsed %ld seconds, %ld useconds\n", diff.tv_sec, diff.tv_usec);

        long int usec = timeval_to_usec(diff);
        printf("Elapsed %ld useconds for change TDF\n", usec);

        return usec;
    }
}

int main(int argc, char const *argv[])
{
	long int usec = repeat_set_tdf_for_myself();
	printf("Avg overhead %f useconds for myself\n", (float)usec / NR_ROUND);

	usec = repeat_set_tdf_for_my_parent();
	printf("Avg overhead %f useconds for my parent\n", (float)usec / NR_ROUND);

	return 0;
}
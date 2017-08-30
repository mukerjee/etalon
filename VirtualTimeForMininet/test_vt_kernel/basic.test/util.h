#include <linux/types.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define NR_ROUND 10000000L
#define MOUNT_SYS_MIN_VERSION "2.6.35"
#define USEC_PER_SEC 1000000
#define CNT_SLEEP 1000000


#define NSCLONEFLGS         \
(                           \
    SIGCHLD         |		\
    CLONE_NEWNS     |		\
    CLONE_NEWUTS    |		\
    CLONE_NEWIPC    |	    \
    CLONE_NEWPID    |		\
    CLONE_NEWNET 		    \
)

void chk_sys_call_ret(long ret, char* sys_cl_nm);
long int timeval_to_usec(struct timeval tv);
int timeval_substract(struct timeval* result, struct timeval* x, struct timeval* y);

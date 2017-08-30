#include "util.h"

void chk_sys_call_ret(long ret, char* sys_cl_nm)
{
    if(ret) {
        printf("\n[error] %s fails with error: %s\n", sys_cl_nm, strerror(errno));
        exit(errno);
    }
}


int timeval_substract(struct timeval* result, struct timeval* x, struct timeval* y)
{
    if (x->tv_usec < y->tv_usec )
    {
        int nsec = (y->tv_usec - x->tv_usec) / USEC_PER_SEC + 1;
        y->tv_usec -= USEC_PER_SEC * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > USEC_PER_SEC)
    {
        int nsec = (x->tv_usec - y->tv_usec) / USEC_PER_SEC + 1;
        y->tv_usec += USEC_PER_SEC * nsec;
        y->tv_sec -= nsec;
    }
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    return x->tv_sec > y->tv_sec;
}

long int timeval_to_usec(struct timeval tv)
{
    return tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
}
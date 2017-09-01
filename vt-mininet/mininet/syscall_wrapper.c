#include <sys/syscall.h>

// int timeclone(unsigned long flags, int dilation)
// {
//     return syscall(317, flags | 0x02000000, 0, NULL, dilation, NULL);
// }

/*
 * hard coded number from syscall_64.tbl
 */
#define GETTIMEOFDAY 96
#define VIRTUALTIMEUNSHARE 318
#define GETVIRTUALTIMEOFDAY 319
#define SETTIMEDILATIONFACTOR 321

int virtualtimeunshare(unsigned long flags, int dilation)
{
    return syscall(VIRTUALTIMEUNSHARE, flags | 0x02000000, dilation);
}


int getvirtualtimeofday(struct timeval *tv, struct timezone *tz)
{
    return syscall(GETVIRTUALTIMEOFDAY, tv, tz);
}


int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    return syscall(GETTIMEOFDAY, tv, tz); // 96 is from syscall_64.tbl
}

/*
 *  ppid == 0 : change caller itself's dilation
 *  ppid !=0 :  change caller's parent's dilation
 */
int settimedilationfactor(int dilation, int ppid)
{
	return syscall(SETTIMEDILATIONFACTOR, dilation, ppid);
}


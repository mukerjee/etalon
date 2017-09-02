#include "syscall_wrapper.h"

#define VIRTUALTIMEUNSHARE 317
#define SETTIMEDILATIONFACTOR 318
#define GETTIMEDILATION 319
#define GETSTARTVIRTUALTIME 320

int virtualtimeunshare(unsigned long flags, int dilation) {
	return syscall(VIRTUALTIMEUNSHARE, flags | 0x02000000, dilation);
}

/*
 *  ppid == 0 : change caller itself's dilation
 *  ppid !=0 :  change caller's parent's dilation
 */
int settimedilationfactor(int dilation, int ppid) {
	return syscall(SETTIMEDILATIONFACTOR, dilation, ppid);
}

int gettimedilation() {
    return syscall(GETTIMEDILATION);
}

int getstartvirtualtime() {
    return syscall(GETSTARTVIRTUALTIME);
}

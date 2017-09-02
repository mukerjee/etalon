#ifndef _SYSCALL_WRAPPER_H_
#define _SYSCALL_WRAPPER_H_

#include <unistd.h>
#include <sys/syscall.h>

/*
 *	This wrapper will ease the invocation of newly added system calls
 *	Think of it as a part of glibc
 */

int virtualtimeunshare(unsigned long, int);
int settimedilationfactor(int, int);
int gettimedilation(void);
long getstartvirtualtime(void);

#endif

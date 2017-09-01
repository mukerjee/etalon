#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 *	This wrapper will ease the invocation of newly added system calls
 *	Think of it as a part of glibc
 */

// int timeclone(unsigned long, unsigned long, int*, int, int*);
int virtualtimeunshare(unsigned long, int);
int getvirtualtimeofday(struct timeval *, struct timezone *);
int gettimeofday(struct timeval *, struct timezone *);
int settimedilationfactor(int, int);


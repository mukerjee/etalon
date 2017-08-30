CC=gcc
FLAGS=-I.
CP=cp

all: test_syscalls accu_gtod ovhd_gtod ovhd_settdf

test_syscalls: wrapper util
	$(CC) $(FLAGS) -o test_syscalls basic.test/test_syscalls.c syscall_wrapper.o util.o

accu_gtod: wrapper util
	$(CC) $(FLAGS) -o accu_gtod basic.test/accu_gtod.c syscall_wrapper.o util.o

ovhd_gtod: wrapper util
	$(CC) $(FLAGS) -o ovhd_gtod basic.test/ovhd_gtod.c syscall_wrapper.o util.o

ovhd_settdf: wrapper util
	$(CC) $(FLAGS) -o ovhd_settdf basic.test/ovhd_settdf.c syscall_wrapper.o util.o


wrapper: syscall.wrap/syscall_wrapper.c
	$(CC) $(FLAGS) -c syscall.wrap/syscall_wrapper.c
	# $(CP) syscall.wrap/syscall_wrapper.c ../mininet/syscall_wrapper.c
	# $(CP) syscall.wrap/syscall_wrapper.h ../mininet/syscall_wrapper.h


util: basic.test/util.c
	$(CC) $(FLAGS) -c basic.test/util.c


.PHONY: clean
clean:
	rm test_syscalls accu_gtod ovhd_gtod ovhd_settdf *.o

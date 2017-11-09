obj-m += tcp_reno_tuner.o tcp_ss.o tcp_ocs.o

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

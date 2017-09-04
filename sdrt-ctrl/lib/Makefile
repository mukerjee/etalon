all:    libs

libs:   sdrt-ctrl.so

sdrt-ctrl.so: sdrt-ctrl.c
	rm -f sdrt-ctrl.so*
	gcc -O3 -fPIC -shared -Werror -Wall -o sdrt-ctrl.so  sdrt-ctrl.c -ldl

clean:
	rm -f sdrt-ctrl.so* 

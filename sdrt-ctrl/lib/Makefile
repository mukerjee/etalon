all:    libs

libs:   sdrt-ctrl-sock.so

sdrt-ctrl-sock.so: sdrt-ctrl-sock.c
	rm -f sdrt-ctrl-sockso*
	gcc -O3 -fPIC -shared -Werror -Wall -o sdrt-ctrl-sock.so  sdrt-ctrl-sock.c -ldl

clean:
	rm -f sdrt-ctrl-sock.so* 

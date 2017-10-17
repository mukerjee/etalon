all: adu-send.so

install: adu-send.so
	cp adu-send.so /usr/lib/

adu-send.so: adu-send.c
	rm -f adu-send.so*
	gcc -O3 -fPIC -shared -Werror -Wall -o adu-send.so  adu-send.c -ldl

clean:
	rm -f adu-send.so* 

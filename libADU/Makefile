all: libADU.so

install: libADU.so
	cp libADU.so /usr/lib/
	cp libADU.so /sdrt/vhost/

libADU.so: libADU.c
	rm -f libADU.so*
	gcc -O3 -fPIC -shared -Werror -Wall -o libADU.so  libADU.c -ldl

clean:
	rm -f libADU.so*

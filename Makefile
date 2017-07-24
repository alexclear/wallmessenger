all: wallmessenger

wallmessenger: wallmessenger.o
	gcc -g wallmessenger.o -l yaml -o wallmessenger

wallmessenger.o: wallmessenger.c typedefs.h
	gcc -g -Wall -Wextra -c wallmessenger.c

clean:
	rm -rf *.o
	rm -rf wallmessenger

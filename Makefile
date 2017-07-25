all: wallmessenger

wallmessenger: wallmessenger.o config_parser.o mylog.o
	gcc -g mylog.o wallmessenger.o config_parser.o -l yaml -o wallmessenger

wallmessenger.o: wallmessenger.c typedefs.h mylog.h
	gcc -g -Wall -Wextra -c wallmessenger.c

config_parser.o: typedefs.h config_parser.c mylog.h
	gcc -g -Wall -Wextra -c config_parser.c

mylog.o: mylog.c mylog.h
	gcc -g -Wall -Wextra -c mylog.c

clean:
	rm -rf *.o
	rm -rf wallmessenger

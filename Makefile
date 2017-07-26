all: wallmessenger

wallmessenger: wallmessenger.o config_parser.o mylog.o processing_loop_single.o processing_loop_multiple_threads.o open_port.o processing_loop_select.o
	gcc -g processing_loop_select.o processing_loop_multiple_threads.o open_port.o processing_loop_single.o mylog.o wallmessenger.o config_parser.o -l yaml -lpthread `pkg-config --libs glib-2.0` -o wallmessenger

wallmessenger.o: wallmessenger.c typedefs.h mylog.h
	gcc -g -Wall -Wextra -c wallmessenger.c

config_parser.o: typedefs.h config_parser.c mylog.h
	gcc -g -Wall -Wextra -c config_parser.c

mylog.o: mylog.c mylog.h
	gcc -g -Wall -Wextra -c mylog.c

open_port.o: open_port.c
	gcc -g -Wall -Wextra -c open_port.c

processing_loop_single.o: processing_loop_single.c
	gcc -g -Wall -Wextra -c processing_loop_single.c

processing_loop_multiple_threads.o: processing_loop_multiple_threads.c
	gcc -g -Wall -Wextra `pkg-config --cflags glib-2.0` -c processing_loop_multiple_threads.c

processing_loop_select.o: processing_loop_select.c
	gcc -g -Wall -Wextra `pkg-config --cflags glib-2.0` -c processing_loop_select.c

clean:
	rm -rf *.o
	rm -rf wallmessenger

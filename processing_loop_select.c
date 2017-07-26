#include "typedefs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <glib.h>
#include <string.h>

#define BACKLOG_LENGTH 10

extern config_t config;

#define ERR_SOCKET -1
#define ERR_BIND -2
#define ERR_LISTEN -3
#define ERR_ACCEPT -4
#define ERR_SHUTDOWN -5
#define ERR_READ -6
#define ERR_THREAD -7

#define BUF_LEN 4096

int do_processing_loop_select(int socket_fd) {
    for (;;) {
        int connect_fd = accept(socket_fd, NULL, NULL);
  
        if (0 > connect_fd) {
            mylog("accept failed: %s", strerror(errno));
            close(socket_fd);
            return ERR_ACCEPT;
        }
  
        // TODO
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

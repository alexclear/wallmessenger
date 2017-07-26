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
#include <sys/select.h>

#define BACKLOG_LENGTH 10

extern config_t config;
GArray* fds;

#define ERR_SOCKET -1
#define ERR_BIND -2
#define ERR_LISTEN -3
#define ERR_ACCEPT -4
#define ERR_SHUTDOWN -5
#define ERR_READ -6
#define ERR_THREAD -7

#define BUF_LEN 4096

int do_processing_loop_select(int socket_fd) {
    fd_set read_fds;
    int number_fds = 0;
    struct timeval tv;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    number_fds++;
    fds = g_array_sized_new (FALSE, TRUE, sizeof(int), 1024);
    char buff[BUF_LEN];

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    for (;;) {
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int number_fds = 0;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        number_fds++;
        int retval = select(number_fds, &read_fds, NULL, NULL, &tv);

        if (retval == -1) {
            mylog("select() failed: %s\n", strerror(errno));
        } else if (retval) {
            mylog("Data is available\n");
            if( FD_ISSET(socket_fd, &read_fds) ) {
                int connect_fd = accept(socket_fd, NULL, NULL);
                FD_SET(connect_fd, &read_fds);
                number_fds++;
                g_array_append_val(fds, connect_fd);
  
                if (0 > connect_fd) {
                    mylog("accept failed: %s", strerror(errno));
                    close(socket_fd);
                    return ERR_ACCEPT;
                }
            }
            int i=0;
            for(; i<fds->len; i++) {
                int client_fd = g_array_index(fds, int, i);
                if( FD_ISSET(client_fd, &read_fds) ) {
                    int result = read(client_fd, buff, BUF_LEN);
                    if( result > 0 ) {
                        char* tempstr = malloc(result+1);
                        strncpy(tempstr, buff, result);
                        tempstr[result] = 0;
                        mylog("[%d] %d bytes read: %s\n", client_fd, result, tempstr);
                        free(tempstr);
                    } else {
                        switch( result ) {
                        case 0:
                            break;
                        default:
                            mylog("Error reading: %d\n", result);
                            return ERR_READ;
                        }
                    }
                }
            }
        } else {
            mylog("No data within five seconds\n");
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

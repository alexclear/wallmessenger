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
    int max_fd = 0;
    struct timeval tv;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    max_fd = socket_fd + 1;
    fds = g_array_sized_new (FALSE, TRUE, sizeof(int), 1024);
    char buff[BUF_LEN];

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    for (;;) {
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        max_fd = socket_fd + 1;
        int i=fds->len - 1;
        for(; i>=0; i--) {
            int client_fd = g_array_index(fds, int, i);
            FD_SET(client_fd, &read_fds);
            if((client_fd + 1) > max_fd) {
                max_fd = client_fd + 1;
            }
        }
        int retval = select(max_fd, &read_fds, NULL, NULL, &tv);

        if (retval < 0) {
            mylog("select() failed %d: %s\n", retval, strerror(errno));
        } else if (retval) {
            mylog("Data is available, %d\n", retval);
            i=0;
            int* remove_list = malloc(fds->len * sizeof(int));
            int remove_list_size = 0;
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
                            mylog("Should close a socket: %d\n", result);
                            remove_list[remove_list_size] = i;
                            remove_list_size++;
                            break;
                        default:
                            mylog("Error reading: %d\n", result);
                            return ERR_READ;
                        }
                    }
                }
            }
            for(i=0; i<remove_list_size; i++) {
                g_array_remove_index(fds, remove_list[i]);
            }
            free(remove_list);
            if( FD_ISSET(socket_fd, &read_fds) ) {
                int connect_fd = accept(socket_fd, NULL, NULL);
                FD_SET(connect_fd, &read_fds);
                if((connect_fd + 1) > max_fd) {
                    max_fd = connect_fd + 1;
                }
                g_array_append_val(fds, connect_fd);
  
                if (0 > connect_fd) {
                    mylog("accept failed: %s", strerror(errno));
                    close(socket_fd);
                    return ERR_ACCEPT;
                }
            }
        } else {
            mylog("No data within five seconds: %d\n", retval);
        }
        mylog("Restarting a loop, fds->len: %d\n", fds->len);
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

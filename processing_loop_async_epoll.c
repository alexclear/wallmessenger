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
#include <sys/epoll.h>
#include <fcntl.h>

#define BACKLOG_LENGTH 10

extern config_t config;
GHashTable* fds;

#define ERR_SOCKET -1
#define ERR_BIND -2
#define ERR_LISTEN -3
#define ERR_ACCEPT -4
#define ERR_SHUTDOWN -5
#define ERR_READ -6
#define ERR_THREAD -7
#define ERR_EPOLL_CREATE -8
#define ERR_EPOLL_CTL -9

#define BUF_LEN 4096
#define MAX_EPOLL_EVENTS 64
#define EPOLL_TIMEOUT_MS 5000

int dummy = 1;

void* key_destroyer(void* key) {
    free(key);
    return NULL;
}

int do_processing_loop_async_epoll(int socket_fd) {
    int max_fd = 0;
    // Создаем файловый дескриптор для механизма epoll
    int epoll_fd = epoll_create1 (0);
    struct epoll_event event;
    struct epoll_event* events;
    if(epoll_fd < 0) {
        mylog("epoll_create1() failed: %s\n", 1, strerror(errno));
        return ERR_EPOLL_CREATE;
    }
    event.data.fd = socket_fd;
    event.events = EPOLLIN | EPOLLET;
    // Добавляем в epoll наш сокет
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event)) {
        mylog("epoll_ctl() failed: %s\n", 1, strerror(errno));
        return ERR_EPOLL_CTL;
    }
    int flags;
    flags = fcntl (socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    fds = g_hash_table_new_full(g_int_hash, g_int_equal, key_destroyer, NULL);
    char buff[BUF_LEN];

    events = calloc (MAX_EPOLL_EVENTS, sizeof event);

    for (;;) {
        int retval = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, EPOLL_TIMEOUT_MS);

        if (retval < 0) {
            mylog("epoll_wait() failed %d: %s\n", 1, retval, strerror(errno));
        } else if (retval) {
            //mylog("Data is available, %d\n", 0, retval);
            int i=0;
            for(; i<retval; i++) {
                if( (events[i].events & EPOLLERR) ||
                      (events[i].events & EPOLLHUP) ||
                      (!(events[i].events & EPOLLIN))) {
                    mylog("epoll failed\n", 1);
                    close(events[i].data.fd);
                    continue;
                }
                if( events[i].data.fd == socket_fd ) {
                    int connect_fd = accept(socket_fd, NULL, NULL);
                    if (0 > connect_fd) {
                        mylog("accept failed: %s", 1, strerror(errno));
                        close(socket_fd);
                        return ERR_ACCEPT;
                    }
                    flags = fcntl (connect_fd, F_GETFL, 0);
                    fcntl(connect_fd, F_SETFL, flags | O_NONBLOCK);
                    event.data.fd = connect_fd;
                    event.events = EPOLLIN;
                    // Добавляем в epoll наш сокет
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &event)) {
                        mylog("epoll_ctl() failed: %s\n", 1, strerror(errno));
                        return ERR_EPOLL_CTL;
                    }
                    int* connect_fd_copy = malloc(sizeof(int));
                    memcpy(connect_fd_copy, &connect_fd, sizeof(int));
                    g_hash_table_insert(fds, connect_fd_copy, &dummy);
                } else {                
                    int result = read(events[i].data.fd, buff, BUF_LEN);
                    if( result > 0 ) {
                        char* tempstr = malloc(result+1);
                        strncpy(tempstr, buff, result);
                        tempstr[result] = 0;
                        //mylog("[%d] %d bytes read: %s\n", 0, events[i].data.fd, result, tempstr);
                        int j=0;
                        GList* elem = g_hash_table_get_keys (fds);
                        GList* first = elem;
                        
                        while (elem != NULL) {
                            int peer_fd = *((int *) elem->data);
                            //mylog("Peer fd: [%d]\n", 0, peer_fd);
                            if(peer_fd != events[i].data.fd) {
                                write(peer_fd, tempstr, strlen(tempstr));
                            }
                            elem = elem->next;
                        }
                        g_list_free(first);
                        free(tempstr);
                    } else {
                        switch( result ) {
                        case 0:
                            mylog("Should close a socket: %d\n", 0, result);
                            g_hash_table_remove(fds, &(events[i].data.fd));
                            close(events[i].data.fd);
                            break;
                        default:
                            mylog("Error reading: %d\n", 1, result);
                            return ERR_READ;
                        }
                    }
                }
            }
        } else {
            mylog("No data within five seconds: %d\n", 0, retval);
        }
        //mylog("Restarting a loop, fds->len: %d\n", 0, g_hash_table_size(fds));
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

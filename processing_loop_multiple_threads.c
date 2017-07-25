#include "typedefs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

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

typedef struct {
    int client_fd;
    pthread_t thread_id;
} thread_context_t;

void *process_client(void* context) {
    char buff[BUF_LEN];
    mylog("Creating a thread: %d, id: %d\n", ((thread_context_t*) context)->client_fd, ((thread_context_t*) context)->thread_id);
    for(;;) {
        int result = read(((thread_context_t*) context)->client_fd, buff, BUF_LEN);
        if( result > 0 ) {
            char* tempstr = malloc(result+1);
            strncpy(tempstr, buff, result);
            tempstr[result] = 0;
            mylog("[%d] [%d] %d bytes read: %s\n", ((thread_context_t*) context)->thread_id, ((thread_context_t*) context)->client_fd, result, tempstr);
            free(tempstr);
        } else {
            switch( result ) {
            case 0:
                break;
            default:
                mylog("Error reading: %d\n", result);
                free(context);
                return NULL;
            }
            break;
        }
    }

    // Если соединение закрыто - освободить ресурсы
    if (shutdown(((thread_context_t*) context)->client_fd, SHUT_RDWR) == -1) {
        perror("shutdown failed");
        close(((thread_context_t*) context)->client_fd);
        free(context);
        return NULL;
    }
    close(((thread_context_t*) context)->client_fd);
    free(context);
    return NULL;
}

int do_processing_loop_multiple_threads() {
    // Открыть порт
    struct sockaddr_in sa;
    int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        perror("cannot create socket");
        return ERR_SOCKET;
    }

    memset(&sa, 0, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(config.port_number));
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
  
    if( bind(socket_fd,(struct sockaddr *)&sa, sizeof sa) == -1 ) {
        perror("bind failed");
        close(socket_fd);
        return ERR_BIND;
    }
  
    // Начать принимать соединения на порту
    if (listen(socket_fd, BACKLOG_LENGTH) == -1) {
        perror("listen failed");
        close(socket_fd);
        return ERR_LISTEN;
    }
  
    for (;;) {
        int connect_fd = accept(socket_fd, NULL, NULL);
  
        if (0 > connect_fd) {
            perror("accept failed");
            close(socket_fd);
            return ERR_ACCEPT;
        }
  
        thread_context_t *context = malloc(sizeof(thread_context_t));
        context->client_fd = connect_fd;
        // Если новое соединение установлено - выделить ему обработчик
        int result = pthread_create(&(context->thread_id), NULL, process_client, context);
        if( result < 0) {
            errno = result;
            perror("pthread_create failed");
            return ERR_THREAD;
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

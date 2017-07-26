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

GHashTable* threads;

typedef struct {
    char* message;
    pthread_t sender;
} iter_context_t;

void threads_iterator(gpointer key, gpointer value, gpointer user_data) {
    iter_context_t* iter_context = (iter_context_t*) user_data;
    thread_context_t* thread_context = (thread_context_t*) value;
    if((*((pthread_t*)key)) == (iter_context->sender ) ) {
        mylog("Don't send to ourselves!\n");
    } else {
        write(thread_context->client_fd, iter_context->message, strlen(iter_context->message));
    }
}

void *process_client(void* context) {
    char buff[BUF_LEN];
    thread_context_t* this_context = (thread_context_t*) context;
    mylog("Creating a thread: %d, id: %d\n", this_context->client_fd, this_context->thread_id);
    for(;;) {
        int result = read(this_context->client_fd, buff, BUF_LEN);
        if( result > 0 ) {
            char* tempstr = malloc(result+1);
            strncpy(tempstr, buff, result);
            tempstr[result] = 0;
            mylog("[%d] [%d] %d bytes read: %s\n", ((thread_context_t*) context)->thread_id, ((thread_context_t*) context)->client_fd, result, tempstr);
            // TODO: Получить блокировку на чтение хэш-таблицы
            iter_context_t iter_context;
            iter_context.message = tempstr;
            iter_context.sender = this_context->thread_id;
            g_hash_table_foreach(threads, (GHFunc)threads_iterator, &iter_context);
            free(tempstr);
        } else {
            switch( result ) {
            case 0:
                break;
            default:
                mylog("Error reading: %d\n", result);
                // TODO: получить блокировку на запись в хэш-таблицу
                // TODO: удалить этот поток из хэш-таблицы
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

    threads = g_hash_table_new(g_int_hash, g_int_equal);

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
        // TODO: получить блокировку на запись в хэш-таблицу
        g_hash_table_insert(threads, &context->thread_id, context);
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

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

typedef struct {
    int client_fd;
    pthread_t thread_id;
} thread_context_t;

GHashTable* threads;
pthread_rwlock_t threads_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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
            // Получить блокировку на чтение хэш-таблицы
            if(pthread_rwlock_rdlock(&threads_rwlock) != 0) {
                mylog("Error getting a read lock!\n");
                exit(EXIT_FAILURE);
            }
            iter_context_t iter_context;
            iter_context.message = tempstr;
            iter_context.sender = this_context->thread_id;
            g_hash_table_foreach(threads, (GHFunc)threads_iterator, &iter_context);
            if(pthread_rwlock_unlock(&threads_rwlock) != 0) {
                mylog("Error unlocking a lock!\n");
                exit(EXIT_FAILURE);
            }
            free(tempstr);
        } else {
            switch( result ) {
            case 0:
                break;
            default:
                mylog("Error reading: %d\n", result);
                // получить блокировку на запись в хэш-таблицу
                if(pthread_rwlock_wrlock(&threads_rwlock) != 0) {
                    mylog("Error getting a write lock!\n");
                    exit(EXIT_FAILURE);
                }
                // удалить этот поток из хэш-таблицы
                g_hash_table_remove(threads, &(this_context->thread_id));
                free(context);
                if(pthread_rwlock_unlock(&threads_rwlock) != 0) {
                    mylog("Error unlocking a lock!\n");
                    exit(EXIT_FAILURE);
                }
                return NULL;
            }
            break;
        }
    }

    // Если соединение закрыто - освободить ресурсы
    if (shutdown(((thread_context_t*) context)->client_fd, SHUT_RDWR) == -1) {
        mylog("shutdown failed: %s", strerror(errno));
        close(((thread_context_t*) context)->client_fd);
        free(context);
        return NULL;
    }
    close(((thread_context_t*) context)->client_fd);
    free(context);
    return NULL;
}

int do_processing_loop_multiple_threads(int socket_fd) {
    threads = g_hash_table_new(g_int_hash, g_int_equal);

    for (;;) {
        int connect_fd = accept(socket_fd, NULL, NULL);
  
        if (0 > connect_fd) {
            mylog("accept failed: %s", strerror(errno));
            close(socket_fd);
            return ERR_ACCEPT;
        }
  
        thread_context_t *context = malloc(sizeof(thread_context_t));
        context->client_fd = connect_fd;
        // Если новое соединение установлено - выделить ему обработчик
        int result = pthread_create(&(context->thread_id), NULL, process_client, context);
        if( result < 0) {
            errno = result;
            mylog("pthread_create failed: %s", strerror(errno));
            return ERR_THREAD;
        }
        // получить блокировку на запись в хэш-таблицу
        if(pthread_rwlock_wrlock(&threads_rwlock) != 0) {
            mylog("Error getting a write lock!\n");
            exit(EXIT_FAILURE);
        }
        g_hash_table_insert(threads, &context->thread_id, context);
        if(pthread_rwlock_unlock(&threads_rwlock) != 0) {
            mylog("Error unlocking a lock!\n");
            exit(EXIT_FAILURE);
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

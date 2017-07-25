#include "typedefs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define BACKLOG_LENGTH 10

extern config_t config;

#define ERR_SOCKET -1
#define ERR_BIND -2
#define ERR_LISTEN -3
#define ERR_ACCEPT -4
#define ERR_SHUTDOWN -5
#define ERR_READ -6

#define BUF_LEN 4096

int do_processing_loop_single() {
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
  
        // Если новое соединение установлено - выделить ему обработчик

        char buff[BUF_LEN];
        for(;;) {
            int result = read(connect_fd, buff, BUF_LEN);
            if( result > 0 ) {
                char* tempstr = malloc(result+1);
                strncpy(tempstr, buff, result);
                tempstr[result] = 0;
                mylog("%d bytes read: %s\n", result, tempstr);
                free(tempstr);
            } else {
                switch( result ) {
                case 0:
                    break;
                default:
                    mylog("Error reading: %d\n", result);
                    return ERR_READ;
                }
                break;
            }
        }

        // Если соединение закрыто - освободить ресурсы
        if (shutdown(connect_fd, SHUT_RDWR) == -1) {
            perror("shutdown failed");
            close(connect_fd);
            close(socket_fd);
            return ERR_SHUTDOWN;
        }
        close(connect_fd);
    }

    close(socket_fd);
    return EXIT_SUCCESS; 
}

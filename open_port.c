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

int open_port() {
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

    return socket_fd; 
}

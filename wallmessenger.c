#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>

#include "typedefs.h"

#define TRUE 1
#define FALSE 0

config_t config;
unsigned reread_config = FALSE;

extern int config_parser();
extern int open_port();
extern int do_processing_loop_single(int socket_fd);
extern int do_processing_loop_multiple_threads(int socket_fd);
extern int do_processing_loop_select(int socket_fd);
extern int do_processing_loop_async_select(int socket_fd);

int start_in_foreground = FALSE;
int conf_pipe[2];

void sighup_handler(int signum) {
    fprintf(stderr, "In a handler\n");
    // Перечитать конфигурационный файл
    reread_config = TRUE;
    char flag = 'a';
    int i = write(conf_pipe[1], &flag, 1);
    if(i == -1) {
        perror("Write failed");
    }
    fprintf(stderr, "Leaving the handler: %d, conf_pipe[1]: %d\n", i, conf_pipe[1]);
}

void* config_reader(void* context) {
    int readfd = *((int*) context);
    mylog("readfd: %d\n", readfd);
    if(parse_config() != 0) {
        exit(EXIT_FAILURE);
    }
    for(;;) {
        char buf;
        read(readfd, &buf, 1);
        mylog("Read from readfd!\n");
        if(reread_config == TRUE) {
            if(parse_config() != 0) {
                mylog("Parsing failed!\n");
            } else {
                mylog("Config replaced!\n");
            }
            reread_config = FALSE;
        }
    }
}

int main(int argc, char* argv[]) {
    int c;

    while ((c = getopt (argc, argv, "fu:p:c:")) != -1)
    switch (c)
    {
    case 'f':
        start_in_foreground = TRUE;
        break;
    case 'u':
        config.user_name = optarg;
        break;
    case 'p':
        config.port_number = optarg;
        break;
    case 'c':
        config.config_path = optarg;
        break;
    case '?':
        if (('c' == optopt) || ('p' == optopt) || (optopt == 'u')) {
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        } else if (isprint (optopt)) {
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        } else {
            fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        }
        exit(EXIT_FAILURE);
    default:
        abort ();
    }

    if(!start_in_foreground) {
        // Сделать fork
        pid_t pid = fork();
        if(pid < 0) {
            mylog("fork failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Если родитель - выйти
        if(pid > 0) {
            exit(EXIT_SUCCESS);
        } else {
            // Если потомок - переоткрыть stdin, stdout, sterr в /dev/null и продолжить работу
            setsid();
            fclose(stderr);
            fclose(stdout);
            fclose(stdin);
            int fd = open("/dev/null",O_RDWR);
            dup(fd);
            dup(fd);
        }
    }

    if(parse_config() != 0) {
        exit(EXIT_FAILURE);
    }

    int socket_fd = open_port();
    if( socket_fd < 0) {
        exit(EXIT_FAILURE);
    }

    // Сбросить права
    struct passwd* euser_info = getpwnam(config.user_name);
    if( euser_info == NULL ) {
        mylog("getpwnam failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if( seteuid(euser_info->pw_uid) != 0 ) {
        mylog("seteuid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pthread_t thread_id;
    if(pipe(conf_pipe) < 0) {
        mylog("pipe() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    mylog("conf_pipe[0]: %d, conf_pipe[1]: %d\n", conf_pipe[0], conf_pipe[1]);
    int result = pthread_create(&thread_id, NULL, config_reader, &(conf_pipe[0]));
    if( result < 0) {
        mylog("pthread_create failed: %s", strerror(result));
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_hup;
    sa_hup.sa_handler = &sighup_handler;

    // Restart the system call, if at all possible
    sa_hup.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigfillset(&sa_hup.sa_mask);

    if(sigaction(SIGHUP, &sa_hup, NULL) != 0) {
        mylog("sigaction failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(config.server_type == SYNC_THREADS) {
        if(do_processing_loop_multiple_threads(socket_fd) != 0) {
            exit(EXIT_FAILURE);
        }
    } else if(config.server_type == ASYNC_SELECT) {
        if(do_processing_loop_async_select(socket_fd) != 0) {
            exit(EXIT_FAILURE);
        }
    } else {
        if(do_processing_loop_select(socket_fd) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}

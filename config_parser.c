#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <string.h>
#include <stdarg.h>

#define PORT_NUMBER "port_number"
#define USER_NAME "user_name"
#define LOGGING_ENABLED "logging_enabled"
#define LOG_FILE_PATH "log_file_path"
#define SERVER_TYPE "server_type"
#define LOGGING_YES "Yes"
#define SERVER_TYPE_SYNC_SELECT "sync_select"
#define SERVER_TYPE_ASYNC_SELECT "async_select"
#define SERVER_TYPE_SYNC_THREADS "sync_threads"
#define SERVER_TYPE_ASYNC_EPOLL "async_epoll"
#define TRUE 1
#define FALSE 0

#include "typedefs.h"
#include "mylog.h"

extern config_t config;

int parse_config() {
    FILE* config_file = fopen(config.config_path, "r");
    if(config_file == NULL) {
        perror("Can't open the config file");
        return -1;
    }

    yaml_parser_t parser;
    /* Create the Parser object. */
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, config_file);
    yaml_event_t event;

    int done=0;
    parser_state_t state=PARSING;

    /* Read the event sequence. */
    while (!done) {

        /* Get the next event. */
        if (!yaml_parser_parse(&parser, &event)) {
           fprintf(stderr, "Bad config file, YAML parser error %d\n", parser.error);
           return -2;
        }

        if(event.type == YAML_SCALAR_EVENT) {
            if(event.data.scalar.value != NULL) {
                fprintf(stderr, "value: %s\n", event.data.scalar.value);
                if(state == NEXT_PORT_NUM) {
                    config.port_number = malloc(event.data.scalar.length + 1);
                    strncpy(config.port_number, event.data.scalar.value, event.data.scalar.length);
                    config.port_number[event.data.scalar.length] = NULL;
                    state = PARSING;
                }
                if(strncmp(PORT_NUMBER, event.data.scalar.value, event.data.scalar.length) == 0) {
                    state = NEXT_PORT_NUM;
                }
                if(state == NEXT_USER_NAME) {
                    config.user_name = malloc(event.data.scalar.length + 1);
                    strncpy(config.user_name, event.data.scalar.value, event.data.scalar.length);
                    config.user_name[event.data.scalar.length] = NULL;
                    state = PARSING;
                }
                if(strncmp(USER_NAME, event.data.scalar.value, event.data.scalar.length) == 0) {
                    state = NEXT_USER_NAME;
                }
                if(state == NEXT_LOG_FILE) {
                    config.log_file_path = malloc(event.data.scalar.length + 1);
                    strncpy(config.log_file_path, event.data.scalar.value, event.data.scalar.length);
                    config.log_file_path[event.data.scalar.length] = NULL;
                    state = PARSING;
                }
                if(strncmp(LOG_FILE_PATH, event.data.scalar.value, event.data.scalar.length) == 0) {
                    state = NEXT_LOG_FILE;
                }
                if(state == NEXT_LOGGING_EN) {
                    char* tempstring = malloc(event.data.scalar.length + 1);
                    strncpy(tempstring, event.data.scalar.value, event.data.scalar.length);
                    tempstring[event.data.scalar.length] = NULL;
                    config.logging_enabled = FALSE;
                    if(strcmp(tempstring, LOGGING_YES) == 0) {
                        config.logging_enabled = TRUE;
                    }
                    free(tempstring);
                    state = PARSING;
                }
                if(strncmp(LOGGING_ENABLED, event.data.scalar.value, event.data.scalar.length) == 0) {
                    state = NEXT_LOGGING_EN;
                }
                if(state == NEXT_SERVER_TYPE) {
                    char* tempstring = malloc(event.data.scalar.length + 1);
                    strncpy(tempstring, event.data.scalar.value, event.data.scalar.length);
                    tempstring[event.data.scalar.length] = NULL;
                    if(strcmp(tempstring, SERVER_TYPE_SYNC_THREADS) == 0) {
                        config.server_type = SYNC_THREADS;
                    } else if(strcmp(tempstring, SERVER_TYPE_SYNC_SELECT) == 0) {
                        config.server_type = SYNC_SELECT;
                    } else if(strcmp(tempstring, SERVER_TYPE_ASYNC_SELECT) == 0) {
                        config.server_type = ASYNC_SELECT;
                    } else if(strcmp(tempstring, SERVER_TYPE_ASYNC_EPOLL) == 0) {
                        config.server_type = ASYNC_EPOLL;
                    } else {
                        mylog("Bad server type value\n");
                        return -4;
                    }
                    free(tempstring);
                    state = PARSING;
                }
                if(strncmp(SERVER_TYPE, event.data.scalar.value, event.data.scalar.length) == 0) {
                    state = NEXT_SERVER_TYPE;
                }
            }
        } 

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        /* The application is responsible for destroying the event object. */
        yaml_event_delete(&event);
    }

    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);

    int result = fclose(config_file);
    if(result != NULL) {
        perror("Can't close the config file");
        return -3;
    }

    mylog("Port number: %s\n", config.port_number);
    mylog("User name: %s\n", config.user_name);
    mylog("Log file path: %s\n", config.log_file_path);
    mylog("Logging enabled: %d\n", config.logging_enabled);
    return 0;
}

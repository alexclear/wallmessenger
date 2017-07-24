#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <string.h>
#include <stdarg.h>
/*
//#define TRUE FALSE // Счастливой отладки!
*/

#define PORT_NUMBER "port_number"
#define USER_NAME "user_name"
#define LOGGING_ENABLED "logging_enabled"
#define LOG_FILE_PATH "log_file_path"
#define LOGGING_YES "Yes"
#define TRUE 1
#define FALSE 0

#include "typedefs.h"

config_t config;

void log(char *fmt, ...) {
    FILE *log = fopen(config.log_file_path, "a");
    if(log == NULL) {
        perror("Can't open the log file");
        exit(EXIT_FAILURE);
    }
    va_list args;
    va_start( args, fmt );
    vfprintf( log, fmt, args );
    va_end( args );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    close( log );
}

int main(int argc, char* argv[]) {
    //int i=0;
    int c;

/*
    for(; i<argc; i++) {
        fprintf(stderr, "Argument #%i: %s\n", i, argv[i]);
    }
*/

    while ((c = getopt (argc, argv, "u:p:c:")) != -1)
    switch (c)
    {
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

    FILE* config_file = fopen(config.config_path, "r");
    if(config_file == NULL) {
        perror("Can't open the config file");
        exit(EXIT_FAILURE);
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
           exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }

    log("Port number: %s\n", config.port_number);
    log("User name: %s\n", config.user_name);
    log("Log file path: %s\n", config.log_file_path);
    log("Logging enabled: %d\n", config.logging_enabled);

    exit(EXIT_SUCCESS);
}

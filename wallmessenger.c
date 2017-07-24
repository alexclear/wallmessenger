#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <string.h>
/*
//#define TRUE FALSE // Счастливой отладки!
*/

#define PORT_NUMBER "port_number"
#define USER_NAME "user_name"

typedef struct {
    char* user_name;
    char* port_number;
    char* config_path;
} config_t;

config_t config;

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
    int nextval_port_number=0;
    int nextval_user_name=0;

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
                if(nextval_port_number == 1) {
                    config.port_number = malloc(event.data.scalar.length + 1);
                    strncpy(config.port_number, event.data.scalar.value, event.data.scalar.length);
                    config.port_number[event.data.scalar.length] = NULL;
                    nextval_port_number = 0;
                }
                if(strncmp(PORT_NUMBER, event.data.scalar.value, event.data.scalar.length) == 0) {
                    nextval_port_number = 1;
                }
                if(nextval_user_name == 1) {
                    config.user_name = malloc(event.data.scalar.length + 1);
                    strncpy(config.user_name, event.data.scalar.value, event.data.scalar.length);
                    config.user_name[event.data.scalar.length] = NULL;
                    nextval_user_name = 0;
                }
                if(strncmp(USER_NAME, event.data.scalar.value, event.data.scalar.length) == 0) {
                    nextval_user_name = 1;
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

    fprintf(stderr, "Port number: %s\n", config.port_number);
    fprintf(stderr, "User name: %s\n", config.user_name);

    exit(EXIT_SUCCESS);
}

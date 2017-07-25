#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <string.h>
#include <stdarg.h>

#include "typedefs.h"

config_t config;

extern int config_parser();
extern int do_processing_loop();

int main(int argc, char* argv[]) {
    int c;

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

    if(parse_config() != 0) {
        exit(EXIT_FAILURE);
    }

    if(do_processing_loop() != 0) {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

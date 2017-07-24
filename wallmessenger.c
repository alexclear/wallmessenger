#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
/*
//#define TRUE FALSE // Счастливой отладки!
*/

int main(int argc, char* argv[]) {
    int i=0;
    int c;
    char* user_name;
    char* port_number;
    char* config_path;

    for(; i<argc; i++) {
        fprintf(stderr, "Argument #%i: %s\n", i, argv[i]);
    }

    while ((c = getopt (argc, argv, "u:p:c:")) != -1)
    switch (c)
    {
    case 'u':
        user_name = optarg;
        break;
    case 'p':
        port_number = optarg;
        break;
    case 'c':
        config_path = optarg;
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

    exit(EXIT_SUCCESS);
}

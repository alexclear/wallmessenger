#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "typedefs.h"

extern config_t config;

void mylog(char *fmt, ...) {
    if(config.logging_enabled) {
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
        fclose( log );
    }
}

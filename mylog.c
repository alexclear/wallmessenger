#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "typedefs.h"

extern config_t config;
FILE *log = NULL;
volatile int reopen = 0;

void mylog(char *fmt, int sync, ...) {
    if(config.logging_enabled) {
        if(reopen) {
            reopen = 0;
            fprintf(log, "Reopening a log\n");
            fclose(log);
            log = NULL;
        }
        if(log == NULL) {
            log = fopen(config.log_file_path, "a");
        }
        if(log == NULL) {
            perror("Can't open the log file");
            exit(EXIT_FAILURE);
        }
        va_list args;
        va_start( args, fmt );
        vfprintf( log, fmt, args );
        va_end( args );
        FILE* console = stdout;
        if(sync) {
            console = stderr;
            fsync( fileno(log) );
        }
        va_start( args, fmt );
        vfprintf( console, fmt, args );
        va_end( args );
    }
}

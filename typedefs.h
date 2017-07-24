#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H 1

typedef enum {
    PARSING,
    NEXT_PORT_NUM,
    NEXT_USER_NAME,
    NEXT_LOGGING_EN,
    NEXT_LOG_FILE
} parser_state_t;

typedef struct {
    char* user_name;
    char* port_number;
    char* config_path;
    char* log_file_path;
    int logging_enabled;
} config_t;

#endif

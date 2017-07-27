#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H 1

typedef enum {
    PARSING,
    NEXT_PORT_NUM,
    NEXT_USER_NAME,
    NEXT_LOGGING_EN,
    NEXT_LOG_FILE,
    NEXT_SERVER_TYPE
} parser_state_t;

typedef enum {
    SYNC_THREADS,
    SYNC_SELECT,
    ASYNC_SELECT
} server_type_t;

typedef struct {
    char* user_name;
    char* port_number;
    char* config_path;
    char* log_file_path;
    int logging_enabled;
    server_type_t server_type;
} config_t;

#endif

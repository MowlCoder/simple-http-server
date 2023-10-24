#ifndef CONFIG_HEADRS
#define CONFIG HEADER

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define BASE_PORT 3000
#define STATIC_BASE_PATH "./static"
#define CONFIG_FILE_PATH "./config.conf"

typedef struct Config {
    int port;
    int request_logging;
    char* static_path;
} Config;

void parse_config(Config* config);

#endif
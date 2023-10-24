#include "config.h"

void parse_config(Config* config) {
    config->port = BASE_PORT;
    config->request_logging = 0;
    config->static_path = STATIC_BASE_PATH;

    FILE* file = fopen(CONFIG_FILE_PATH, "r");

    if (file == NULL) {
        perror("ERROR: can not parse config file");
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "=");

        if (strcmp(key, "PORT") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "REQUEST_LOGGING") == 0) {
            config->request_logging = atoi(value);
        } else if (strcmp(key, "STATIC_PATH") == 0) {
            config->static_path = (char*)malloc(strlen(value)+1);
            strcpy(config->static_path, value);
        }
    }

    fclose(file);
}
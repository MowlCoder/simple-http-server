#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"
#include "time.h"

#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "sys/socket.h"
#include "netinet/in.h"

#include "config.h"

#define NOT_FOUND "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"
#define METHOD_NOT_ALLOWED "HTTP/1.1 405 Method not allowed\r\nConnection: close\r\n\r\n"
#define INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal server error\r\nConnection: close\r\n\r\n"

typedef struct HTTPRequest {
    char* method;
    char* path;
} HTTPRequest;

typedef struct HTTPResponse {
    int status_code;
    const char* status_text;
} HTTPResponse;

Config config;

HTTPRequest parse_request(char* buffer) {
    HTTPRequest req;

    char* first_line = strtok_r(buffer, "\n", &buffer);

    req.method = strtok_r(first_line, " ", &first_line);
    req.path = strtok_r(first_line, " ", &first_line);

    return req;
}

const char* get_content_type(const char* file_name) {
    const char* dot = strrchr(file_name, '.');

    if (dot == NULL || dot == file_name) {
        return "";
    }

    const char* file_ext = dot + 1;

    if (strcmp(file_ext, "html") == 0 || strcmp(file_ext, "htm") == 0) {
        return "text/html";
    } else if (strcmp(file_ext, "css") == 0) {
        return "text/css";
    } else if (strcmp(file_ext, "js") == 0) {
        return "text/javascript";
    } else if (strcmp(file_ext, "jpg") == 0 || strcmp(file_ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(file_ext, "png") == 0) {
        return "image/png";
    } else if (strcmp(file_ext, "svg") == 0) {
        return "image/svg+xml";
    } else {
        return "";
    }
}

char* get_static_file_path(const char* request_path) {
    size_t file_path_len = strlen(request_path);
    size_t base_path_len = strlen(config.static_path);
    char* file_path = (char*)malloc(file_path_len+base_path_len+1);

    if (file_path == NULL) {
        return NULL;
    }

    strcpy(file_path, config.static_path);
    strcpy(file_path+base_path_len, request_path);

    return file_path;
}

void send_not_found(HTTPResponse* response, int client_socket_fd) {
    response->status_code = 404;
    response->status_text = "Not Found";
    send(client_socket_fd, NOT_FOUND, strlen(NOT_FOUND), 0);
}

void send_method_not_allowed(HTTPResponse* response, int client_socket_fd) {
    response->status_code = 405;
    response->status_text = "Method not allowed";
    send(client_socket_fd, METHOD_NOT_ALLOWED, strlen(METHOD_NOT_ALLOWED), 0);
}

void send_internal_server_error(HTTPResponse* response, int client_socket_fd) {
    response->status_code = 500;
    response->status_text = "Internal server error";
    send(client_socket_fd, INTERNAL_SERVER_ERROR, strlen(INTERNAL_SERVER_ERROR), 0);
}

void send_static_file(HTTPRequest* request, HTTPResponse* response, int client_socket_fd, const char* filename) {
    int file_fd = open(filename, O_RDONLY);

    if (file_fd == -1) {
        send_not_found(response, client_socket_fd);
        return;
    }

    char response_buffer[2048];
    memset(response_buffer, 0, sizeof(response_buffer));

    struct stat file_stat;

    if (fstat(file_fd, &file_stat) != 0) {
        send_internal_server_error(response, client_socket_fd);
        return;
    }

    snprintf(response_buffer, sizeof(response_buffer),
        "HTTP/1.1 200 OK\r\nServer: MowlCod-C\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n",
        (long)file_stat.st_size, get_content_type(filename)
    );
    send(client_socket_fd, response_buffer, strlen(response_buffer), 0);

    char buffer[2048];
    int bytes_read;

    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        send(client_socket_fd, buffer, bytes_read, 0);
    }

    response->status_code = 200;
    response->status_text = "OK";

    close(file_fd);
}

void* handle_client(void* arg) {
    clock_t before = clock();
    int client_fd = *((int*)arg);

    char* buffer = (char *)malloc(2048 * sizeof(char));
    ssize_t bytes_received = recv(client_fd, buffer, 2048, 0);

    HTTPRequest request = parse_request(buffer);
    HTTPResponse response;

    if (strcmp(request.method, "GET") != 0) {
        send_method_not_allowed(&response, client_fd);
    } else {
        if (strrchr(request.path, '.') == NULL) {
            char* index_file_path = get_static_file_path("/index.html");
            send_static_file(&request, &response, client_fd, index_file_path);
            free(index_file_path);
        } else {
            char* file_path = get_static_file_path(request.path);
            send_static_file(&request, &response, client_fd, file_path);
            free(file_path);
        }
    }

    close(client_fd);

    if (config.request_logging) {
        double ms = (double)(clock() - before) * 1000.0 / CLOCKS_PER_SEC;
        printf("INFO: %s %s - %d %s (%.2f ms)\n", request.method, request.path, response.status_code, response.status_text, ms);
    }

    free(buffer);
    free(arg);

    return NULL;
}

int main(void) {
    parse_config(&config);

    printf("INFO: Starting web server...\n");

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("ERROR: Can not create socket file descriptor");
        return 1;
    }

    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("ERROR: Can not set sock opt SO_REUSEADDR");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Can not bind socket to port");
        return 1;
    }

    if (listen(socket_fd, 10) < 0) {
        perror("ERROR: Can not start listening");
        return 1;
    }

    printf("INFO: Web server is started on port %d\n", config.port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_fd < 0) {
            perror("ERROR: Failed to accept connection from client");
            continue;
        }

        int* client_fd_arg = (int*)malloc(sizeof(int));
        if (client_fd_arg == NULL) {
            perror("ERROR: Failed to allocate memory for thread argument");
            close(client_fd);
            continue;
        }

        *client_fd_arg = client_fd;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_fd_arg);
        pthread_detach(thread_id);
    }

    close(socket_fd);

    return 0;
}
#ifndef PROXY_H
#define PROXY_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#define LISTENING_PORT 8080
#define BUFFER_SIZE 65536

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_dt;

typedef enum {
    FAILED = -1,
    SUCCESS = 0,
} my_error_code;

// Из proxy_server.c
extern atomic_int proxy_running;
extern int server_socket_fd;

int start_proxy_server(int port);
void stop_proxy_server(void);
void signal_handler(int sig);

// Из network_utils.c
int create_server_socket(int port);
int connect_to_server(const char *host, int port);

// Из http_parser.c
int parse_address(char *buffer, char *host, int *port);
int read_until_end(int sockfd, char *buffer, size_t buffer_size, ssize_t *total_read);

// Из client_handler.c
void *handle_client(void *arg);
void *forward_client_to_server(void *args);
int receive_first_request(int client_sock, char *buffer, ssize_t *bytes_read);
int send_first_request(int server_sock, char *buffer, ssize_t bytes_read);
int forward_data(int src_fd, int dst_fd);

#endif  // PROXY_H
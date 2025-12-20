#include "proxy.h"

static volatile int proxy_running = 1;
static int server_socket_fd = -1;
static pthread_mutex_t running_mutex;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    stop_proxy_server();  
}

int create_server_socket(int port){
    int sockfd;
    struct sockaddr_in serv_addr = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd < 0){
        perror("ERROR create sockfd");
        return FAILED;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0){
        perror("ERROR in bind");
        return FAILED;
    }

     if (listen(sockfd, 10) < 0) {
        perror("ERROR on listen");
        close(sockfd);
        return FAILED;
    }

    return sockfd; 
}

int parse_address(char *buffer, char *host, int *port) {
    char *substring = NULL;
    char *host_start = NULL;
    char *host_end = NULL;
    
    substring = strstr(buffer, "http://");
    if (substring) {
        host_start = substring + 7; 
        
        host_end = host_start;
        while (*host_end && *host_end != ':' && *host_end != '/' && 
               *host_end != ' ' && *host_end != '\r' && *host_end != '\n') {
            host_end++;
        }
        
        int len = host_end - host_start;
        if (len >= 256) return FAILED;
        strncpy(host, host_start, len);
        host[len] = '\0';
        
        if (*host_end == ':') {
            *port = atoi(host_end + 1);
            if (*port <= 0) *port = 80;
        } else {
            *port = 80;
        }
        
        return SUCCESS;
    }
    
    substring = strstr(buffer, "Host:");
    if (substring) {
        host_start = substring + 5; 
        

        while (*host_start == ' ') host_start++;
        
        host_end = host_start;
        while (*host_end && *host_end != ':' && *host_end != ' ' && 
               *host_end != '\r' && *host_end != '\n') {
            host_end++;
        }

        int len = host_end - host_start;
        if (len >= 256) return FAILED;
        strncpy(host, host_start, len);
        host[len] = '\0';
        

        if (*host_end == ':') {
            *port = atoi(host_end + 1);
            if (*port <= 0) *port = 80;
        } else {
            *port = 80;
        }
        
        return SUCCESS;
    }
    
    return FAILED;
}

void *handle_client(void *arg) {
    client_dt *cli = (client_dt *)arg;  
    char buffer[BUFFER_SIZE];
    char port_str[16]; 

    memset(buffer, 0, BUFFER_SIZE);

    ssize_t bytes_read = recv(cli->client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; 
    }
    else if (bytes_read <= 0) {
        perror("ERROR in recv"); 
        close(cli->client_socket);
        free(cli);
        return NULL;
    }
    
    char host[256];
    int port = 80;  

    if (parse_address(buffer, host, &port) != 0) {
        fprintf(stderr, "ERROR Failed to parse address\n");
        close(cli->client_socket);
        free(cli);
        return NULL; 
    }
    
    printf("[Thread %lu] Request: %s:%d\n", (unsigned long)pthread_self(), host, port);

    snprintf(port_str, sizeof(port_str), "%d", port);
    
    struct addrinfo hints = {0}, *result = NULL;
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "Error: getaddrinfo failed for %s: %s\n", 
                host, gai_strerror(ret));
        close(cli->client_socket);
        free(cli);
        return NULL;
    }
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("ERROR creating server socket");
        freeaddrinfo(result);
        close(cli->client_socket);
        free(cli);
        return NULL;
    }
    
    if (connect(server_sock, result->ai_addr, result->ai_addrlen) < 0) {
        perror("ERROR connecting to remote server");
        freeaddrinfo(result);
        close(server_sock);
        close(cli->client_socket);
        free(cli);
        return NULL;
    }
    
    freeaddrinfo(result);
    
    char *ptr = buffer;
    ssize_t remaining = bytes_read;
    while (remaining > 0) {
        ssize_t sent = send(server_sock, ptr, remaining, 0);
        if (sent < 0) {
            perror("ERROR sending to server");
            close(server_sock);
            close(cli->client_socket);
            free(cli);
            return NULL;
        }
        remaining -= sent;
        ptr += sent;
    }
    
    ssize_t n;
    while ((n = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        char *resp_ptr = buffer;
        ssize_t resp_remaining = n;
        
        while (resp_remaining > 0) {
            ssize_t sent = send(cli->client_socket, resp_ptr, resp_remaining, 0);
            if (sent < 0) {
                perror("ERROR sending to client");
                break;
            }
            resp_remaining -= sent;
            resp_ptr += sent;
        }
        
        if (resp_remaining > 0) {
            break;  
        }
    }
    
    close(server_sock);
    close(cli->client_socket);
    free(cli);
    return NULL;
}

int start_proxy_server(int port) {
     struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        return FAILED;
    }

    server_socket_fd = create_server_socket(port); 
    if (server_socket_fd < 0) {
        return  FAILED;
    }
    printf("HTTP Proxy started on port %d\n", port);
    printf("Press Ctrl+C to stop...\n");

    struct sockaddr_in client_address; 
    socklen_t client_addr_len = sizeof(client_address);
    int client_sock;

     while (proxy_running) {
        client_sock =  accept(server_socket_fd, (struct sockaddr *)&client_address, &client_addr_len);
        if (client_sock < 0) {
             if (proxy_running) {
            perror("ERROR on accept");
             }
             else {
                break;  
            }
    }

         /* if (!proxy_running) {
            if (client_sock >= 0) close(client_sock);
            break;
        } */
        
        printf("New connection from %s:%d\n",
            inet_ntoa(client_address.sin_addr),
            ntohs(client_address.sin_port));

        //для потока
        client_dt* cli  = (client_dt *)malloc(sizeof(client_dt));

        if (!cli) {
            perror("ERROR malloc");
            close(client_sock);
            continue;
        }

        cli->client_socket = client_sock;
        cli->client_addr = client_address;

        pthread_t tid;
        if (pthread_create(&tid, NULL,handle_client, (void *)cli) != 0) {
            perror ("ERROR creating thread");
            free(cli);
            close(client_sock);
            continue;
        }
            pthread_detach(tid);
        

    }

    close(server_socket_fd);
    return SUCCESS;
}

void stop_proxy_server() {
    printf("\nStopping proxy server...\n");
    pthread_mutex_lock(&running_mutex);
    proxy_running = 0;
    pthread_mutex_unlock(&running_mutex);
    
    if (server_socket_fd >= 0) {
        shutdown(server_socket_fd, SHUT_RDWR);
        close(server_socket_fd);
        server_socket_fd = -1;
    }
}




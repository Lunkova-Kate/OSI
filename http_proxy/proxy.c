#include "proxy.h"

#include <netinet/tcp.h>

static volatile int proxy_running = 1;
static int server_socket_fd = -1;
static pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

static void safe_write(const char *msg)
{
    size_t len = 0;
    while (msg[len] != '\0') len++;
    ssize_t bytes_written = write(STDERR_FILENO, msg, len);
    (void)bytes_written;
}

void signal_handler(int sig)
{
    int saved_errno = errno;
    const char *msg = "\nReceived signal, shutting down...\n";
    safe_write(msg);
    stop_proxy_server();
    errno = saved_errno;
}

int create_server_socket(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr = {0};
    int opt = 1;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd < 0)
    {
        perror("ERROR create sockfd");
        return FAILED;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("ERROR setsockopt SO_REUSEADDR");
        close(sockfd);
        return FAILED;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR in bind");
        close(sockfd);
        return FAILED;
    }

    if (listen(sockfd, 10) < 0)
    {
        perror("ERROR on listen");
        close(sockfd);
        return FAILED;
    }

    return sockfd;
}

int parse_address(char *buffer, char *host, int *port)
{
    char *substring = NULL;
    char *host_start = NULL;
    char *host_end = NULL;

    substring = strstr(buffer, "http://");
    if (substring)
    {
        host_start = substring + 7;

        host_end = host_start;
        while (*host_end && *host_end != ':' && *host_end != '/' &&
               *host_end != ' ' && *host_end != '\r' && *host_end != '\n')
        {
            host_end++;
        }

        int len = host_end - host_start;
        if (len >= 256) return FAILED;
        strncpy(host, host_start, len);
        host[len] = '\0';

        if (*host_end == ':')
        {
            *port = atoi(host_end + 1);
            if (*port <= 0) *port = 80;
        }
        else
        {
            *port = 80;
        }

        return SUCCESS;
    }

    substring = strstr(buffer, "Host:");
    if (substring)
    {
        host_start = substring + 5;

        while (*host_start == ' ') host_start++;

        host_end = host_start;
        while (*host_end && *host_end != ':' && *host_end != ' ' &&
               *host_end != '\r' && *host_end != '\n')
        {
            host_end++;
        }

        int len = host_end - host_start;
        if (len >= 256) return FAILED;
        strncpy(host, host_start, len);
        host[len] = '\0';

        if (*host_end == ':')
        {
            *port = atoi(host_end + 1);
            if (*port <= 0) *port = 80;
        }
        else
        {
            *port = 80;
        }

        return SUCCESS;
    }

    return FAILED;
}

void *handle_client(void *arg)
{
    client_dt *cli = (client_dt *)arg;
    char buffer[BUFFER_SIZE];
    char port_str[16];
    ssize_t bytes_read = 0;

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = recv(cli->client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            break;
        }
        else if (bytes_read == 0)
        {
            printf("[Thread %lu] Connection closed by client\n",
                   (unsigned long)pthread_self());
            close(cli->client_socket);
            free(cli);
            return NULL;
        }
        else
        {
            if (errno == EINTR)
            {
                pthread_mutex_lock(&running_mutex);
                int should_stop = (proxy_running == 0);
                pthread_mutex_unlock(&running_mutex);

                if (should_stop)
                {
                    printf(
                        "[Thread %lu] Shutdown requested, closing connection\n",
                        (unsigned long)pthread_self());
                    close(cli->client_socket);
                    free(cli);
                    return NULL;
                }
                continue;
            }
            else
            {
                perror(" error recv");
                close(cli->client_socket);
                free(cli);
                return NULL;
            }
        }
    }

    char host[256];
    int port = 80;

    if (parse_address(buffer, host, &port) != 0)
    {
        fprintf(stderr, "ERROR Failed to parse address\n");
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    printf("[Thread %lu] Request: %s:%d\n", (unsigned long)pthread_self(), host,
           port);

    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints = {0}, *result = NULL, *rp = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(host, port_str, &hints, &result);

    if (ret != 0)
    {
        fprintf(stderr, "Error: getaddrinfo failed for %s: %s\n", host,
                gai_strerror(ret));
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    int server_sock = -1;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        server_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_sock < 0)
        {
            continue;
        }

        int flag = 1;
        if (setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, &flag,
                       sizeof(flag)) < 0)
        {
            perror("WARNING: Could not set TCP_NODELAY");
        }

        if (setsockopt(cli->client_socket, IPPROTO_TCP, TCP_NODELAY, &flag,
                       sizeof(flag)) < 0)
        {
            perror("WARNING: Could not set TCP_NODELAY on client socket");
        }

        if (connect(server_sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close(server_sock);
        server_sock = -1;
    }

    freeaddrinfo(result);
    if (server_sock < 0)
    {
        perror("ERROR connecting to remote server");
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    char *ptr = buffer;
    ssize_t remaining = bytes_read;
    while (remaining > 0)
    {
        ssize_t sent = send(server_sock, ptr, remaining, 0);
        if (sent < 0)
        {
            if (errno == EINTR)
            {
                pthread_mutex_lock(&running_mutex);
                int should_stop = (proxy_running == 0);
                pthread_mutex_unlock(&running_mutex);

                if (should_stop)
                {
                    close(server_sock);
                    close(cli->client_socket);
                    free(cli);
                    return NULL;
                }
                continue;
            }
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
    ssize_t total_received = 0;
    while ((n = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        char *resp_ptr = buffer;
        ssize_t resp_remaining = n;

        while (resp_remaining > 0)
        {
            ssize_t sent =
                send(cli->client_socket, resp_ptr, resp_remaining, 0);
            if (sent < 0)
            {
                if (errno == EINTR)
                {
                    pthread_mutex_lock(&running_mutex);
                    int should_stop = (proxy_running == 0);
                    pthread_mutex_unlock(&running_mutex);

                    if (should_stop)
                    {
                        break;
                    }
                    continue;
                }
                perror("ERROR sending to client");
                break;
            }
            resp_remaining -= sent;
            resp_ptr += sent;
        }

        if (resp_remaining > 0)
        {
            break;
        }
    }

    if (n < 0 && errno != EINTR)
    {
        perror("ERROR receiving from server");
    }

    close(server_sock);
    close(cli->client_socket);
    free(cli);
    return NULL;
}

int start_proxy_server(int port)
{
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("sigaction");
        return FAILED;
    }

    server_socket_fd = create_server_socket(port);
    if (server_socket_fd < 0)
    {
        return FAILED;
    }
    int flag = 1;
    if (setsockopt(server_socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag,
                   sizeof(flag)) < 0)
    {
        perror("WARNING: Could not set TCP_NODELAY on server socket");
    }
    printf("HTTP Proxy started on port %d\n", port);
    printf("Press Ctrl+C to stop...\n");

    struct sockaddr_in client_address;
    socklen_t client_addr_len = sizeof(client_address);
    int client_sock;

    while (1)
    {
        client_sock =
            accept(server_socket_fd, (struct sockaddr *)&client_address,
                   &client_addr_len);
        if (client_sock < 0)
        {
            pthread_mutex_lock(&running_mutex);
            int still_running = proxy_running;
            pthread_mutex_unlock(&running_mutex);

            if (!still_running)
            {
                break;
            }

            if (errno == EINTR)
            {
                continue;
            }

            perror("ERROR on accept");
            continue;
        }

        pthread_mutex_lock(&running_mutex);
        int still_running = proxy_running;
        pthread_mutex_unlock(&running_mutex);

        if (!still_running)
        {
            close(client_sock);
            break;
        }

        printf("New connection from %s:%d\n",
               inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));

        client_dt *cli = (client_dt *)malloc(sizeof(client_dt));
        if (!cli)
        {
            perror("ERROR malloc");
            close(client_sock);
            continue;
        }

        cli->client_socket = client_sock;
        cli->client_addr = client_address;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)cli) != 0)
        {
            perror("ERROR creating thread");
            free(cli);
            close(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    if (server_socket_fd >= 0)
    {
        close(server_socket_fd);
        server_socket_fd = -1;
    }

    return SUCCESS;
}

void stop_proxy_server()
{
    pthread_mutex_lock(&running_mutex);
    proxy_running = 0;
    pthread_mutex_unlock(&running_mutex);
    if (server_socket_fd >= 0)
    {
        //shutdown(server_socket_fd, SHUT_RDWR);
        close(server_socket_fd);
        server_socket_fd = -1;
        sleep(2);
    }
}

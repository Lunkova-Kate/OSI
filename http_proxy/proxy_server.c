#include "proxy.h"
#include <netinet/tcp.h> 

atomic_int proxy_running = 1;
int server_socket_fd = -1;

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

    while (atomic_load(&proxy_running))
    {
        struct pollfd pfd;
        pfd.fd = server_socket_fd;
        pfd.events = POLLIN;
        
        int ret = poll(&pfd, 1, 1000);
        
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                if (atomic_load(&proxy_running) == 0)
                {
                    break;
                }
                continue;
            }
            perror("ERROR in poll");
            break;
        }
        else if (ret == 0)
        {
            continue;
        }
        
        client_sock = accept(server_socket_fd, (struct sockaddr *)&client_address,
                           &client_addr_len);
        if (client_sock < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("ERROR on accept");
            continue;
        }

        flag = 1;
        if (setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &flag,
                       sizeof(flag)) < 0)
        {
            perror("WARNING: Could not set TCP_NODELAY on client socket");
        }

        printf("New connection from %s:%d\n",
               inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));

        client_dt *cli = malloc(sizeof(client_dt));
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

    printf("Proxy server stopped\n");
    return SUCCESS;
}

void stop_proxy_server(void)
{
    atomic_store(&proxy_running, 0);
    
    if (server_socket_fd >= 0)
    {
        close(server_socket_fd);
        server_socket_fd = -1;
    }
}
#include "proxy.h"
#include <netinet/tcp.h>

int receive_first_request(int client_sock, char *buffer, ssize_t *bytes_read)
{
    int result = read_until_end(client_sock, buffer, BUFFER_SIZE, bytes_read);
    
    if (result == SUCCESS && *bytes_read > 0)
    {
        buffer[*bytes_read] = '\0';
        return SUCCESS;
    }
    else if (*bytes_read == 0)
    {
        printf("[Thread %lu] Connection closed by client\n",
               (unsigned long)pthread_self());
        return FAILED;
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to read complete HTTP header\n");
        return FAILED;
    }
}

int send_first_request(int server_sock, char *buffer, ssize_t bytes_read)
{
    char *ptr = buffer;
    ssize_t remaining = bytes_read;
    
    while (remaining > 0)
    {
        ssize_t sent = send(server_sock, ptr, remaining, 0);
        if (sent < 0)
        {
            if (errno == EINTR)
            {
                if (atomic_load(&proxy_running) == 0)
                {
                    return -1;
                }
                continue;
            }
            perror("ERROR sending to server");
            return -1;
        }
        remaining -= sent;
        ptr += sent;
    }
    
    return 0;
}

int forward_data(int src_fd, int dst_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t n;
    
    while ((n = recv(src_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        char *resp_ptr = buffer;
        ssize_t resp_remaining = n;

        while (resp_remaining > 0)
        {
            ssize_t sent = send(dst_fd, resp_ptr, resp_remaining, 0);
            if (sent < 0)
            {
                if (errno == EINTR)
                {
                    if (atomic_load(&proxy_running) == 0)
                    {
                        return -1;
                    }
                    continue;
                }
                perror("ERROR sending data");
                return -1;
            }
            resp_remaining -= sent;
            resp_ptr += sent;
        }
    }

    if (n < 0 && errno != EINTR)
    {
        perror("ERROR receiving data");
        return -1;
    }

    return 0;
}

void *forward_client_to_server(void *args)
{
    int *fds = (int *)args;
    int client_fd = fds[0];
    int server_fd = fds[1];
    
    char local_buffer[BUFFER_SIZE];
    ssize_t n;
    
    while ((n = recv(client_fd, local_buffer, BUFFER_SIZE, 0)) > 0)
    {
        char *ptr = local_buffer;
        ssize_t remaining = n;
        
        while (remaining > 0)
        {
            ssize_t sent = send(server_fd, ptr, remaining, 0);
            if (sent < 0)
            {
                if (errno == EINTR)
                {
                    if (atomic_load(&proxy_running) == 0)
                    {
                        free(args);
                        return NULL;
                    }
                    continue;
                }
                perror("ERROR sending to server");
                free(args);
                return NULL;
            }
            remaining -= sent;
            ptr += sent;
        }
    }
    
    free(args);
    return NULL;
}

void *handle_client(void *arg)
{
    client_dt *cli = (client_dt *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = 0;

    if (receive_first_request(cli->client_socket, buffer, &bytes_read) != SUCCESS)
    {
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    char host[256];
    int port = 80;
    if (parse_address(buffer, host, &port) != SUCCESS)
    {
        fprintf(stderr, "ERROR Failed to parse address\n");
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    printf("[Thread %lu] Request: %s:%d\n", (unsigned long)pthread_self(), host,
           port);

    int server_sock = connect_to_server(host, port);
    if (server_sock < 0)
    {
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    if (send_first_request(server_sock, buffer, bytes_read) != SUCCESS)
    {
        close(server_sock);
        close(cli->client_socket);
        free(cli);
        return NULL;
    }

    pthread_t forward_thread;
    int *fds = malloc(2 * sizeof(int));
    if (fds)
    {
        fds[0] = cli->client_socket;
        fds[1] = server_sock;
        
        if (pthread_create(&forward_thread, NULL, forward_client_to_server, fds) == 0)
        {
            pthread_detach(forward_thread);
        }
        else
        {
            free(fds);
        }
    }
    
    forward_data(server_sock, cli->client_socket);
    
    close(server_sock);
    close(cli->client_socket);
    free(cli);
    
    return NULL;
}
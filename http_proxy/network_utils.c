#include "proxy.h"
#include <netinet/tcp.h>

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

int connect_to_server(const char *host, int port)
{
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints = {0}, *result = NULL, *rp = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0)
    {
        fprintf(stderr, "Error: getaddrinfo failed for %s: %s\n", host,
                gai_strerror(ret));
        return -1;
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

        if (connect(server_sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close(server_sock);
        server_sock = -1;
    }

    freeaddrinfo(result);
    return server_sock;
}
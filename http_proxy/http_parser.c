#include "proxy.h"

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

int read_until_end(int sockfd, char *buffer, size_t buffer_size, ssize_t *total_read)
{
    char *ptr = buffer;
    size_t total = 0;
    size_t capacity = buffer_size - 1;
    
    while (total < capacity)
    {
        ssize_t n = recv(sockfd, ptr + total, capacity - total, 0);
        
        if (n > 0)
        {
            total += n;
            ptr[total] = '\0';
            
            if (strstr(ptr, "\r\n\r\n") != NULL)
            {
                *total_read = total;
                return SUCCESS;
            }
        }
        else if (n == 0)
        {
            *total_read = total;
            return (total > 0) ? SUCCESS : FAILED;
        }
        else
        {
            if (errno == EINTR)
            {
                if (atomic_load(&proxy_running) == 0)
                {
                    return FAILED;
                }
                continue;
            }
            perror("ERROR in recv");
            return FAILED;
        }
    }
    
    *total_read = total;
    return (strstr(ptr, "\r\n\r\n") != NULL) ? SUCCESS : FAILED;
}
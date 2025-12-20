#ifndef PROXY_H
#define PROXY_H
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>  

#include <signal.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sys/types.h>



#define LISTENING_PORT 8080
#define BUFFER_SIZE 65536 


typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
}client_dt;

typedef enum {
   FAILED = -1,
   SUCCESS = 0,
}my_error_code;

int create_server_socket(int port);
void *handle_client(void *arg);
int  parse_address(char *buffer, char *host, int *port);
int start_proxy_server(int port);
void stop_proxy_server();


#endif // PROXY_H

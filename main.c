#include <stdio.h>
#include <arpa/inet.h> // funções de conversão de endereços IP
#include <string.h>
#include <netinet/in.h> // estruturas de dados para endereços de rede
#include <sys/socket.h> // funções de socket
#include <unistd.h> // funções de manipulação de descritores de arquivo
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>


#define MAXTHREADS 100
#define MAXPORT 65535

struct ThreadArgs {
    char *ip;
    int port;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int active_threads = 0;

char *st_connect(char *ip, int port){
    enum PortStatus { PORT_OPEN, PORT_CLOSED, PORT_FILTERED };
    struct sockaddr_in server;
    int sockfd;
    int status = PORT_CLOSED;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return "CLOSED";
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    int conn = connect(sockfd, (struct sockaddr *)&server, sizeof(server));
    if (conn == 0) {
        close(sockfd);
        return "OPEN";
    }

    if (conn < 0 && errno != EINPROGRESS) {
        close(sockfd);
        return "CLOSED";
    }

    fd_set writefds;
    struct timeval timeout;
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = 2; 
    timeout.tv_usec = 0;

    int res = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (res > 0) {
        if (FD_ISSET(sockfd, &writefds)) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
                status = PORT_CLOSED;
            } else {
                status = (so_error == 0) ? PORT_OPEN : PORT_CLOSED;
            }
        } else {
            status = PORT_FILTERED;
        }
    } else if (res == 0) {
        status = PORT_FILTERED;
    } else {
        status = PORT_FILTERED;
    }

    close(sockfd);
    if (status == PORT_OPEN) return "OPEN";
    if (status == PORT_CLOSED) return "CLOSED";
    return "FILTERED";
}

void *ThreadScan(void *arg){
    pthread_mutex_lock(&mutex);
    active_threads++;
    pthread_mutex_unlock(&mutex);
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    char *result = st_connect(args->ip, args->port);
    pthread_mutex_lock(&mutex);
    printf("Port %d is %s\n", args->port, result);
    active_threads--;
    pthread_mutex_unlock(&mutex);
    free(args);
    return NULL;
}

int main(int argc, char **argv){
    
    if(argc != 3 && argc != 4){
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        printf("Usage: %s <IP> <INITIAL_PORT> <FINAL_PORT>\n", argv[0]);
        return 1;
    }

    if(argc == 4){
        char *ip = argv[1];
        int initial_port = atoi(argv[2]);
        int final_port = atoi(argv[3]);
        for(int port = initial_port; port <= final_port; port++){
            while (active_threads >= MAXTHREADS) {
                usleep(100000); 
            }
            struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
            args->ip = ip;
            args->port = port;
            pthread_t thread;
            pthread_create(&thread, NULL, ThreadScan, args);
            pthread_detach(thread);
            
        }

        while (1) {
            pthread_mutex_lock(&mutex);
            if (active_threads == 0) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            pthread_mutex_unlock(&mutex);
            usleep(50000); 
        }
    }

    else{
        char *ip = argv[1];
        int port = atoi(argv[2]);
        printf("Port %d is %s\n", port, st_connect(ip, port));
    }
    return 0;

}

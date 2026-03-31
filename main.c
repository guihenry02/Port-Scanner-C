#include <stdio.h>
#include <arpa/inet.h> // funções de conversão de endereços IP
#include <string.h>
#include <netinet/in.h> // estruturas de dados para endereços de rede
#include <sys/socket.h> // funções de socket
#include <unistd.h> // funções de manipulação de descritores de arquivo
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>


char *st_connect(char *ip, int port){
    struct sockaddr_in server;
    int sockfd;
    //socket ipv4,tcp
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // Define o socket como não bloqueante

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    
    int result = connect(sockfd, (struct sockaddr *)&server, sizeof(server));   
    fd_set fd_set;
    struct timeval timeout;
    FD_ZERO(&fd_set);
    FD_SET(sockfd, &fd_set);
    timeout.tv_sec = 2; // Tempo limite de 2 segundos
    timeout.tv_usec = 0;

    int res = select(sockfd + 1, NULL, &fd_set, NULL, &timeout);
    if (res > 0) {
        int so_error;
        socklen_t len = sizeof(so_error);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error == 0) {
            result = 0; // Conexão bem-sucedida
        } else {
            result = -1; // Erro na conexão
        }
    } else if (res == 0) {
        result = -1; // Tempo limite atingido
    } else {
        result = -1; // Erro no select

    }

    close(sockfd);
    return result == 0 ? "OPEN" : "CLOSED";
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
            printf("Port %d is %s\n", port, st_connect(ip, port));
        }
    }

    else{
        char *ip = argv[1];
        int port = atoi(argv[2]);
        printf("Port %d is %s\n", port, st_connect(ip, port));
    }
    return 0;

}
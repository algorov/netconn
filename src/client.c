//
// Created by Sergey Mullin on 22.11.2023.
//
//
// Created by Sergey Mullin on 22.11.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>


#define IP "127.0.0.1"
#define PORT 9696


void errorHandler(char *message) {
    perror(message);
    exit(1);
}

int main() {
    int sock;
    struct sockaddr_in address;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[-] Error creating socket!\n");
        exit(1);
    }
    printf("[+] TCP server socket created!\n");

    // Address settings.
    memset(&address, '\n', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = PORT;
    address.sin_addr.s_addr = inet_addr(IP);


    if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("[-] Connection error!\n");
        exit(1);
    }
    printf("[+] Connected to the server!\n");


//    while (1) {
//        char *buffer[256];

    while (1) {
        char buffer[1024];
//        bzero(buffer, 1024);
//        recv(sock, buffer, sizeof(buffer), 0);
//        printf("%s", buffer);

        fgets(buffer, 1024, stdin);
        send(sock, buffer, strlen(buffer), 0);


    }
//    sleep(10);
//        recv(sock, buffer, sizeof(buffer), 0);
//        printf("Hey!");
//    }

//    char buffer[1024];
//    bzero(buffer, 1024);
//    recv(sock, buffer, sizeof(buffer), 0);

    close(sock);
    printf("[+] Disconnected from server!\n\n");

    return 0;
}
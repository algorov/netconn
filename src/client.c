//
// Created by Sergey Mullin on 22.11.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>

#define IP "127.0.0.1"
#define PORT_1 9696
#define PORT_2 9697
#define MILESTONE 5

unsigned long latestRequestTime = 0;

unsigned long getCurTime() {
    return time(NULL);
}

void errorHandler(char *message) {
    perror(message);
    exit(1);
}

void setSocketNonblock(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

void setTimeOutOpt(int socket, void *value, socklen_t value_len) {
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - RCVTIMEO");
    }
}

int Select(int fd_set_size, fd_set *fd_set_buffer_r, fd_set *fd_set_buffer_w, fd_set *fd_set_buffer_e,
           struct timeval *timeout_val) {
    int status;
    if ((status = select(fd_set_size, fd_set_buffer_r, fd_set_buffer_w, fd_set_buffer_e, timeout_val)) == -1) {
        errorHandler("Select error!");
    }

    return status;
}

bool getRequest(int client, char *buffer) {
    struct timeval timeout = {
            .tv_sec = 1,
            .tv_usec = 0,
    };

    fd_set set;
    FD_ZERO(&set);
    FD_SET(client, &set);

    if (Select(client + 1, &set, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(client, &set)) {
            memset(buffer, '\0', 1024);
            read(client, buffer, 1024);

            return true;
        }
    }

    return false;
}

int main() {
    int choice;
    int clientSocket;
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '\n', sizeof(serverAddress));

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    printf("\nКуда подключаемся?\n"
           "[1] Сервер №1\n"
           "[2] Сервер №2\n"
           ">>> ");
    scanf("%d", &choice);

    if (choice == 1) {
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(PORT_1);
        serverAddress.sin_addr.s_addr = inet_addr(IP);
    } else if (choice == 2) {
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(PORT_2);
        serverAddress.sin_addr.s_addr = inet_addr(IP);
    } else {
        printf("Неверный выбор.\n");
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("[-] Connection error!\n");
        exit(1);
    }
    printf("[+] Connected to the server!\n");


//    char *buffer[1024];
//    int nread = 0;
//    bzero(buffer, 1024);
//    while ((nread = recv(clientSocket, buffer, sizeof(buffer), 0) == 0)) {
//    }
//    printf("%s", buffer);
//    nread = 0;
//
//    bzero(buffer, 1024);
//    fgets(buffer, 1024, stdin);
//    send(clientSocket, buffer, strlen(buffer), 0);
//
//    bzero(buffer, 1024);
//    nread = 0;
//    while ((nread = read(clientSocket, buffer, sizeof(buffer)) == 0)) {
//    }
//    printf("%s", buffer);
//
//    unsigned long curTime;
//
//    while (1) {
//        bzero(buffer, 1024);
//        printf(">>> ");
//        fgets(buffer, 1024, stdin);
//
//        if (((curTime = getCurTime()) - latestRequestTime) < MILESTONE) {
//            printf("Таймер ещё не истек!\n");
//            continue;
//        }
//        latestRequestTime = curTime;
//        send(clientSocket, buffer, strlen(buffer), 0);
//
//        bzero(buffer, 1024);
//        nread = 0;
//        while ((nread = read(clientSocket, buffer, sizeof(buffer)) == 0)) {}
//        if (strstr(buffer, "504") != NULL) {
//            break;
//        }
//        printf("%s", buffer);
//    }
    char *buffer[1024];

    while (1) {
        if (getRequest(clientSocket, buffer)) {
            printf("%s\n", buffer);
            if (strstr(buffer, "514") != NULL) {
                printf("Exit\n");
                break;
            }
        }

        bzero(buffer, 1024);
        printf(">> ");
        fgets(buffer, 1024, stdin);
        send(clientSocket, buffer, strlen(buffer), 0);
    }

    return 0;
}
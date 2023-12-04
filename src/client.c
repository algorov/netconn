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

#define INFO_HEADER_COLOR "\033[1;33;40m"
#define GREEN_THEME_COLOR "\033[1;32;40m"
#define MESSAGE_COLOR "\033[0m"
#define ERROR_HEADER_COLOR "\033[1;31;40m"

#define IP "127.0.0.1"
#define PORT_1 9696
#define PORT_2 9697

#define MILESTONE 2

void prettyPrint(char *msg, bool isError);

void errorHandler(char *message);

unsigned long getCurTime();

int Select(int fd_set_size, fd_set *fd_set_buffer_r, fd_set *fd_set_buffer_w, fd_set *fd_set_buffer_e,
           struct timeval *timeout_val);

bool getRequest(int client, char *buffer);

unsigned long latestRequestTime = 0;

void prettyPrint(char *msg, bool isError) {
    if (isError) {
        printf("%s[-]%s %s\n", ERROR_HEADER_COLOR, MESSAGE_COLOR, msg);
    } else {
        printf("%s[+]%s %s %s\n", INFO_HEADER_COLOR, GREEN_THEME_COLOR, msg, MESSAGE_COLOR);
    }
}

void errorHandler(char *message) {
    prettyPrint(message, true);
    exit(1);
}

unsigned long getCurTime() {
    return time(NULL);
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
    int server;
    struct sockaddr_in address;
    memset(&address, '\n', sizeof(address));

    server = socket(AF_INET, SOCK_STREAM, 0);

    int choice;
    printf("Выберите из перечня интересующий вас сервер:\n"
           "[1] Server №1\n"
           "[2] Server №2\n"
           ">>> ");

    scanf("%d", &choice);
    if (choice == 1) {
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT_1);
        address.sin_addr.s_addr = inet_addr(IP);
    } else if (choice == 2) {
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT_2);
        address.sin_addr.s_addr = inet_addr(IP);
    } else {
        errorHandler("Incorrect.");
    }

    if (connect(server, (struct sockaddr *) &address, sizeof(address)) < 0) {
        errorHandler("Connection error!");
    }
    prettyPrint("Connected to the server!\n", false);

    char *buffer[1024];
    unsigned long curTime;

    while (1) {
        if (getRequest(server, buffer)) {
            printf("%s\n", buffer);
            if (strstr(buffer, "514") != NULL) {
                prettyPrint("Exit.", false);
                break;
            }
        }

        bzero(buffer, 1024);
        printf(">> ");
        fgets(buffer, 1024, stdin);

        if (((curTime = getCurTime()) - latestRequestTime) < MILESTONE) {
            prettyPrint("Таймер не истёк!\n", true);
            continue;
        } else {
            latestRequestTime = curTime;
        }

        send(server, buffer, strlen(buffer), 0);
    }

    close(server);

    return 0;
}
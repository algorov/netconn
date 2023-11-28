//
// Created by Sergey Mullin on 28.11.2023.
//
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATCH "./fifo"
#define SERVER_NAME_ONE "ServerOne"
#define SERVER_NAME_TWO "ServerTwo"

int fifoFd;
int servOneLogFd, servTwoLogFd;

void ender(int signal) {
    if (SIGINT == signal) {
        close(fifoFd);
        unlink(FIFO_PATCH);

        close(servOneLogFd);
        close(servTwoLogFd);
    }

    exit(0);
}

int main() {
    signal(SIGINT, ender);

    if (mkfifo(FIFO_PATCH, 0666) == -1) {
        perror("Не смогли создать канал");
        exit(1);
    }
    printf("[+] Канал создан!\n");

    if ((fifoFd = open(FIFO_PATCH, O_RDONLY)) == -1) {
        perror("Не смогли открыть канал");
        exit(1);
    }

    // создаем логи для каждого сервера
    if ((servOneLogFd = open("./../../logs/ServerOne.log", O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
        perror("Не смогли создать логгер");
    }
    lseek(servOneLogFd, 0, SEEK_END);

    if ((servTwoLogFd = open("./../../logs/ServerTwo.log", O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
        perror("Не смогли создать логгер");
    }
    lseek(servTwoLogFd, 0, SEEK_END);


    char *buffer[1024];
    memset(buffer, '\0', 1024);

    int nread;
    while (1) {
        if ((nread = read(fifoFd, buffer, 1024)) != 0) {
            if (strstr(buffer, SERVER_NAME_ONE) != NULL) {
                if (servOneLogFd != -1) {
                    write(servOneLogFd, buffer, strlen(buffer));
                }
            } else if (strstr(buffer, SERVER_NAME_TWO) != NULL) {
                if (servTwoLogFd != -1) {
                    write(servTwoLogFd, buffer, strlen(buffer));
                } else {
                    printf("Что-то другое...\n");
                }
            }

            nread = 0;
            memset(buffer, '\0', 1024);
        }
    }
}
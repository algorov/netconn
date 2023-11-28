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

char *fifoPath = "./fifo";
char *serv1 = "./server.out";

int fd;

void ender(int signal) {
    if (SIGINT == signal) {
        close(fd);
        unlink(fifoPath);
    }

    exit(0);
}

int main() {
    signal(SIGINT, ender);

    if (mkfifo(fifoPath, 0666) == -1) {
        perror("Не смогли создать канал");
    }
    printf("[+] Канал создан!\n");

    if ((fd = open(fifoPath, O_RDONLY)) == -1) {
        perror("Не смогли открыть канал");
        exit(1);
    }

    int fd_1;
    if ((fd_1 = open("./one.log", O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
        perror("Не смогли создать логгер");
    }
    off_t offset = lseek(fd_1, 0, SEEK_END);

    char *buffer[1024];
    memset(buffer, '\0', 1024);

    int nread;
    while (1) {
        if ((nread = read(fd, buffer, 1024)) != 0) {
            printf("%s\n", buffer);

            if (strstr(buffer, serv1) != NULL) {
                if (fd_1 != -1) {
                    write(fd_1, buffer, strlen(buffer));
                }
            } else {
                printf("Иной сервер\n");
            }

            memset(buffer, '\0', 1024);
            nread = 0;
        }
    }

    return 0;
}
//
// Created by Sergey Mullin on 28.11.2023.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define DEFAULT_CAPACITY 1024

#define FIFO_PATH "./fifo"
#define LOGS_DIR_PATH "./../../logs/"
#define FIRST_LOG_NAME "server_one"
#define SECOND_LOG_NAME "server_two"

void removeSubstring(char *str, char *sub);

void addCurrentDateTime(char *buffer);

int openLog(char *path, bool clear);

void clearBuffer(char *buffer, int buffer_capacity);

void prettyPrint(char *msg, bool isError);

void makeFifo(char *path);

int openLog(char *path, bool clear);

void logging(int fd, char *record);

void ender(int signal);


int fifoFd;
int logFdOne, logFdTwo;


void removeSubstring(char *str, char *sub) {
    size_t len = strlen(sub);
    while (str == strstr(str, sub)) {
        memmove(str, str + len, strlen(str + len) + 1);
    }
}

void addCurrentDateTime(char *buffer) {
    int hours, minutes, seconds, day, month, year;
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    hours = local->tm_hour;
    minutes = local->tm_min;
    seconds = local->tm_sec;

    day = local->tm_mday;
    month = local->tm_mon + 1;
    year = local->tm_year + 1900;

    char *dateTime[22];
    clearBuffer(dateTime, 22);
    sprintf((char *) dateTime, "[%02d/%02d/%d %02d:%02d:%02d]", day, month, year, hours, minutes, seconds);
    strcat(buffer, dateTime);
}

void prettyPrint(char *msg, bool isError) {
    if (isError) {
        printf("[-] %s\n", msg);
    } else {
        printf("[+] %s\n", msg);
    }
}

void clearBuffer(char *buffer, int buffer_capacity) {
    memset(buffer, '\0', buffer_capacity);
}

void makeFifo(char *path) {
    if (mkfifo(path, 0666) == -1) {
        prettyPrint("Канал не создан :(", true);
        perror("#");
        exit(1);
    }
    prettyPrint("Канал создан.", false);
}

int openFifo(char *path) {
    int fd;
    if ((fd = open(path, O_RDONLY)) == -1) {
        prettyPrint("Канал не открылся :(", true);
        perror("#");
        exit(1);
    } else {
        prettyPrint("Канал открыт.", false);
    }

    return fd;
}

void buildLogPath(char *buffer, char *fileName) {
    strcat(buffer, LOGS_DIR_PATH);
    strcat(buffer, fileName);
    strcat(buffer, ".log");
}

int openLog(char *path, bool clear) {
    int fd;
    if (clear) {
        if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
            prettyPrint("Логгер не открылся :(", true);
            perror("#");
        } else {
            prettyPrint("Логгер открыт.", false);
        }
    } else {
        if ((fd = open(path, O_CREAT | O_WRONLY, 0666)) == -1) {
            prettyPrint("Логгер не открылся :(", true);
            perror("#");
        } else {
            prettyPrint("Логгер открыт.", false);
            lseek(fd, 0, SEEK_END);
        }
    }

    return fd;
}

void logging(int fd, char *record) {
    char *buffer[DEFAULT_CAPACITY];
    clearBuffer(buffer, DEFAULT_CAPACITY);

    if (fd != -1) {
        addCurrentDateTime(buffer);
        strcat(buffer, " : ");
        strcat(buffer, record);
        strcat(buffer, "\n");
        printf("[*] Записал: %zd букв.\n", write(fd, buffer, strlen(buffer)));
    }
}

void ender(int signal) {
    if (2 == signal) {
        printf("\n");
        close(fifoFd);
        unlink(FIFO_PATH);
        prettyPrint("Канал закрыт и удален.", false);

        close(logFdOne);
        close(logFdTwo);
        prettyPrint("Логи закрыты.", false);
    }

    exit(0);
}

int main() {
    signal(2, ender);

    makeFifo(FIFO_PATH);
    fifoFd = openFifo(FIFO_PATH);

    char *array[DEFAULT_CAPACITY];
    clearBuffer(array, DEFAULT_CAPACITY);
    buildLogPath(array, FIRST_LOG_NAME);
    logFdOne = openLog(array, true);

    clearBuffer(array, DEFAULT_CAPACITY);
    buildLogPath(array, SECOND_LOG_NAME);
    logFdTwo = openLog(array, true);

    int nread;
    clearBuffer(array, DEFAULT_CAPACITY);
    while (1) {
        if ((nread = read(fifoFd, array, DEFAULT_CAPACITY)) != 0) {
            if (strstr(array, FIRST_LOG_NAME) != NULL) {
                removeSubstring(array, FIRST_LOG_NAME);
                logging(logFdOne, array);
            } else if (strstr(array, SECOND_LOG_NAME) != NULL) {
                removeSubstring(array, SECOND_LOG_NAME);
                logging(logFdTwo, array);
            } else {
                prettyPrint("Что-то другое...", false);
            }

            nread = 0;
            clearBuffer(array, DEFAULT_CAPACITY);
        }
    }
}
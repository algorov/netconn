//
// Created by Sergey Mullin on 22.11.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>


#define LOCKER_PATH "./lock/serverOne.lock"
#define MAX_CLIENT 5
#define IP "127.0.0.1"
#define PORT 9696

void setUnlock();

char *FONT_COLOR = {"\033[1;32;40m"};

int lockerFd;

bool isAlive = true;

fd_set readableFd;
int serverSocket;

pthread_t clients[MAX_CLIENT];
int connClientCount = 0;

void errorHandler(char *message) {
    char *buffer = malloc(sizeof(char) * (4 + 50));
    memset(buffer, '\0', sizeof(buffer));


    strcat(buffer, "[-] ");
    strcat(buffer, message);
    perror(buffer);

    free(&buffer);
    setUnlock(lockerFd);
    exit(1);
}

// Подключение клиента.
void addClient(pthread_t *client) {
    clients[connClientCount++] = *client;

//        for (int i = 0; i < MAX_CLIENT; i++) {
//            printf("Add client: %ld ", clients[i]);
//        }
//        printf("\n");
}

void message(char *message) {
    printf("[+] %s.\n", message);
}

// Обёртки.
int Socket(int domain, int type, int protocol) {
    int fd;
    if ((fd = socket(domain, type, protocol)) == -1) {
        errorHandler("[-] Error creating socket");
    }
    message("TCP server socket created");

    return fd;
}

// Binds a socket to a specific address (IP + PORT).
void Bind(int server_socket, struct sockaddr *server_address, int server_address_size) {
    if (bind(server_socket, server_address, server_address_size) < 0) {
        errorHandler("Bind error");
    }

    printf("[+] Bind to the address: %s:%d.\n", IP, PORT);
}

// Start listening on the port.
void Listen(int server_socket, int count) {
    if (listen(server_socket, count) < 0) {
        errorHandler("Listen");
    }

    message("Listening");
}

// Прослушивание множества на предмета активизации.
int Select(int fd_set_size, fd_set *fd_set_buffer_r, fd_set *fd_set_buffer_w, fd_set *fd_set_buffer_e,
           struct timeval *timeout_val) {
    int status;
    if ((status = select(fd_set_size, fd_set_buffer_r, fd_set_buffer_w, fd_set_buffer_e, timeout_val)) == -1) {
        errorHandler("Select");
    }

    return status;
}

// Принятия входящего подключения.
int Accept(int server_socket, struct sockaddr *client_address, socklen_t *client_address_size) {
    int client_fd;
    return ((client_fd = accept(server_socket, (struct sockaddr *) client_address, client_address_size)) > 2)
           ? client_fd : -1;
}

// Установка блокировки на создание дополнительного экземпляра
void setLock(int locker_fd) {
    if ((locker_fd = open(LOCKER_PATH, O_CREAT | O_RDWR, 0666)) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            message("сервер уже запущен");
        } else {
            fprintf(stderr, "Ошибка открытия файла блокировки: %s\n", strerror(errno));
        }
        exit(1);
    }

    // Установка блокировки, если файл открылся.
    if (flock(locker_fd, LOCK_EX | LOCK_NB) == -1) {
        printf("Сервер уже запущен\n");
        exit(1);
    }

    message("Сервер запустился, блокировка установлена");
}

// Убирает блокировку на создание дополнительного экземпляра
void setUnlock(int locker_fd) {
    close(locker_fd);
    unlink(LOCKER_PATH);
    message("Блокировка убрана");
}

// Инициализация сокета
void Init(int *server_socket) {
    setLock(lockerFd);
    *server_socket = Socket(AF_INET, SOCK_STREAM, 0);
}

void endServer() {
    message("Interrupt: CTRL+C");

    isAlive = false;
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] != (void *) 0) {
            pthread_join(clients[i], NULL);
        }
    }

//    pthread_mutex_destroy();
    setUnlock(lockerFd);
    close(serverSocket);

    exit(0);
}

// Опция позволяет установить таймер на прослушивание.
void setTimeOutOpt(int socket, void *value, socklen_t value_len) {
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - RCVTIMEO");
    }
}

// Опция, позволяющая переиспользовать адрес.
void setReuseAddrOpt(int socket, void *value, socklen_t value_len) {
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - REUSEADDR");
    }
}

// Установка сокета в неблокирующий режим.
void setSocketNonblock(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}


/*
 * Individual tasks
 * */
// Изменят цвет шрифта.
bool setFontColor(int code) {
    switch (code) {
        // Default
        case 0:
            FONT_COLOR = "\033[1;0;0m\n";
            break;
        case 1:
            FONT_COLOR = "\033[1;47;30m\n";
            break;
        case 2:
            FONT_COLOR = "\033[1;102;30m\n";
            break;
        case 3:
            FONT_COLOR = "\033[1;101;30m\n";
            break;
        case 4:
            FONT_COLOR = "\033[1;103;30m\n";
            break;
        case 5:
            FONT_COLOR = "\033[1;104;30m\n";
            break;
        case 6:
            FONT_COLOR = "\033[1;43;33m\n";
            break;
        default:
            return false;
            break;
    }

    return true;
}

// Возвращает код цвета шрифта.
void getFontColor(char *buffer) {
    printf("\033[1;0;0m   %s OKEY   \033[1;0;0m\n", FONT_COLOR);
    memset(buffer, '\0', sizeof(buffer) / sizeof(char));
    strcat(buffer, FONT_COLOR);
    strcat(buffer, "\n");
    strcat(buffer, "\033[1;0;0m");
}

// Определяет текущую дату со временем
char *getCurrentDateTime() {
    int hours, minutes, seconds, day, month, year;
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    hours = local->tm_hour;
    minutes = local->tm_min;
    seconds = local->tm_sec;

    day = local->tm_mday;
    month = local->tm_mon + 1;
    year = local->tm_year + 1900;


    char *datetime[22];
    memset(datetime, '\0', sizeof(datetime) / sizeof(char));
    sprintf((char *) datetime, "[%02d/%02d/%d %02d:%02d:%02d]", day, month, year, hours, minutes, seconds);

    return (char *) datetime;
}

void clearBuffer(char *buffer) {
    memset(buffer, '\0', sizeof(buffer) / sizeof(char));
}

void buildNewResponse(char *response_buffer, char *response) {
    clearBuffer(response_buffer);
    strcat(response_buffer, getCurrentDateTime());
    strcat(response_buffer, ": ");
    strcat(response_buffer, response);
    strcat(response_buffer, "\n");
}

void sendResponse(int *client, char *response) {
    send(*client, response, strlen(response), 0);
}

void getRequest(int *client, char *buffer) {
    recv(*client, buffer, sizeof(buffer), 0);
}

void *clientHandler(void *argc) {
    int clientSocket = *(int *) argc;
    printf("[+] Client [%d] connected!\n", clientSocket);

    char *helpInfo = "привет!\n"
                     "[1] - Получение цвета шрифта\n"
                     "[2] - Изменение цвета шрифта\n";

    char request[1024];
    char responseBuffer[1024];
    char response[64];


    while (isAlive) {
        clearBuffer(request);
        getRequest(&clientSocket, request);
        send(clientSocket, helpInfo, strlen(helpInfo), 0);
        clearBuffer(request);
        getRequest(&clientSocket, request);

        switch ((int) strtol(request, NULL, 10)) {
            case 1:
                getFontColor(response);
                buildNewResponse(responseBuffer, response);
                sendResponse(&clientSocket, responseBuffer);
                break;
            case 2:
                buildNewResponse(responseBuffer, "Какой ты хочешь цвет? Вводи только от 0 до 6");
                sendResponse(&clientSocket, responseBuffer);
                clearBuffer(request);
                getRequest(&clientSocket, request);

                int select = -1;
                int index = strcspn(request, "\n");
                if (request[index] == '\n') {
                    request[index] = '\0';
                }

                sscanf(request, "%d", &select);
                if (setFontColor(select)) {
                    buildNewResponse(responseBuffer, "Успех!");
                } else {
                    buildNewResponse(responseBuffer, "Ошибка!");
                }
                sendResponse(&clientSocket, responseBuffer);
                break;
            default:
                buildNewResponse(responseBuffer, "Нет такой операции!");
                sendResponse(&clientSocket, responseBuffer);
                break;
        }
    }

    close(clientSocket);
    printf("[+] Client [%d] disconnected!\n", clientSocket);
//    free(&clientSocket);

    return NULL;
}

// Driver.
int main() {
    Init(&serverSocket);
    signal(SIGINT, endServer);

    // Sets socket property.
    struct timeval timeout = {
            .tv_sec = 4,
            .tv_usec = 0,
    };
    setTimeOutOpt(serverSocket, (char *) &timeout, sizeof(timeout));
    int optFlag = 1;
    setReuseAddrOpt(serverSocket, &optFlag, sizeof(optFlag));
//    setSocketNonblock(serverSocket);

    // Address settings.
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = PORT;
    serverAddress.sin_addr.s_addr = inet_addr(IP);
    Bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    Listen(serverSocket, MAX_CLIENT);

    // Настройка множества дескрпиторов.
    FD_ZERO(&readableFd);
    FD_SET(serverSocket, &readableFd);
    fd_set actual_fdSet;
    int status;

    int clientSocket;
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize;

    while (1) {
        actual_fdSet = readableFd;
        status = Select(FD_SETSIZE, &actual_fdSet, NULL, NULL, &timeout);
        if (status > 0) {
            for (int quessDesc = 0; quessDesc < FD_SETSIZE; quessDesc++) {
                if (FD_ISSET(quessDesc, &actual_fdSet)) {
                    // If new connection.
                    if (serverSocket == quessDesc) {
                        if (connClientCount < MAX_CLIENT) {
                            clientAddressSize = sizeof(clientAddress);
                            if ((clientSocket = Accept(serverSocket, (struct sockaddr *) &clientAddress,
                                                       &clientAddressSize)) != -1) {
                                int *newConnect = malloc(sizeof(int));
                                *newConnect = clientSocket;

                                pthread_t handler;
                                if (pthread_create(&handler, NULL, clientHandler, newConnect) == 0) {
                                    addClient(&handler);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
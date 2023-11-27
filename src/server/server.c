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


#define MAX_CLIENT 5
#define IP "127.0.0.1"
#define PORT 9696

void setUnlock();

// [TODO] При завершении процесса мьютех уничтожить.
// pthread_mutex_lock(&mutex);
// unlock
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

const char *lockfile = "./lock/serverOne.lock";
int locker_fd;


bool isAlive = true;

fd_set readableFd;
int serverSocket;

pthread_t clients[MAX_CLIENT];
int connClientCount = 0;

void errorHandler(char *message) {
    setUnlock();
    char *buffer = malloc(sizeof(char) * (4 + 50));
    memset(buffer, '\0', sizeof(buffer));

    strcat(buffer, "[-] ");
    strcat(buffer, message);
    perror(buffer);

    free(&buffer);
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

void *clientHandler(void *argc) {
    int clientSocket = *(int *) argc;
    printf("[+] Client [%d] connected!\n", clientSocket);
    char buffer[1024];

    while (isAlive) {
        // [TODO] Реализовать peer-to-peer коммуникацию.
//        bzero(buffer, 1024);
//        fgets(buffer, 1024, stdin);
//        send(clientSocket, buffer, strlen(buffer), 0);

        bzero(buffer, 1024);
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Client: %s", buffer);

        sleep(3);
    }

    close(clientSocket);
    printf("[+] Client [%d] disconnected!\n", clientSocket);
//    free(&clientSocket);

    return NULL;
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
void setLock() {
    // Пытаемся открыть файл.
    int fd;
    if ((fd = open(lockfile, O_CREAT | O_RDWR, 0666)) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            message("сервер уже запущен");
        } else {
            fprintf(stderr, "Ошибка открытия файла блокировки: %s\n", strerror(errno));
        }
        exit(1);
    }

    // Установка блокировки, если файл открылся.
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        printf("Сервер уже запущен\n");
        exit(1);
    }

    message("Сервер запустился, блокировка установлена");
}

// убирает блокировку
void setUnlock() {
    close(locker_fd);
    unlink(lockfile);
    message("Блокировка убрана");
}

// Инициализация сокета
void Init(int *server_socket) {
    setLock();
    *server_socket = Socket(AF_INET, SOCK_STREAM, 0);
}

//void setSocketProperty()

void endServer() {
    message("Interrupt: CTRL+C");

    isAlive = false;
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i] != (void *) 0) {
            pthread_join(clients[i], NULL);
        }
    }

//    pthread_mutex_destroy();
    setUnlock();
    close(serverSocket);

    exit(0);
}


// Driver.
int main() {
    Init(&serverSocket);
    signal(SIGINT, endServer);

    int clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressSize;
//    memset(&clients, '\0', sizeof(pthread_t) * MAX_CLIENT);

    // Создает сокет.
//    serverSocket = malloc(sizeof(int));

    // Sets socket property.
    struct timeval timeout = {
            .tv_sec = 4,
            .tv_usec = 0,
    };
    if (setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        printf("sdas");
        errorHandler("SETSOCKOPT - RCVTIMEO");
    }

    int optFlag = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optFlag, sizeof(optFlag)) < 0) {
        errorHandler("SETSOCKOPT - REUSEADDR");
    }

    // Установка сокета в неблокирующий режим.
    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

    // Address settings.
    memset(&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = PORT;
    serverAddress.sin_addr.s_addr = inet_addr(IP);

    Bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    Listen(serverSocket, 5);

//     Настройка множества дескрпиторов для отслеживания.
    FD_ZERO(&readableFd);
    FD_SET(serverSocket, &readableFd);
    fd_set actual_fdSet;
    int status;

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
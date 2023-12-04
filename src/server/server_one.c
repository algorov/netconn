//
// Created by Sergey Mullin on 01.12.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define INFO_HEADER_COLOR "\033[1;33;40m"
#define GREEN_THEME_COLOR "\033[1;32;40m"
#define MESSAGE_COLOR "\033[0m"
#define ERROR_HEADER_COLOR "\033[1;31;40m"


#define DEFAULT_CAPACITY 1024

#define NAME "server_one"
#define MAX_CLIENT 10

#define LOCKER_PATH "./lock/server_one.lock"
#define LOGGER_FIFO_PATH "./../logger/fifo"

#define IP "127.0.0.1"
#define PORT 9696


// ================= HELPERS =================
void prettyPrint(char *msg, bool isError);

void errorHandler(char *msg);

void Info(char *msg);

void clearBuffer(char *buffer, int buffer_capacity);

void addCurDatetime(char *buff);


// ================= LOGGER =================
int getLogger();

void logging(int logger_fd, char *msg);

void closeLogger(int logger_fd);


// ================= LOCK/UNLOCK =================
int setLock();

void setUnlock(int locker_fd);


// ================= DECORATORS =================
int Socket(int domain, int type, int protocol);

void Bind(int server_socket, struct sockaddr *server_address, int server_address_size);

void Listen(int server_socket, int capacity);

int Select(int fd_set_size, fd_set *fd_set_buffer_r, fd_set *fd_set_buffer_w, fd_set *fd_set_buffer_e,
           struct timeval *timeout_val);

int Accept(int server_socket, struct sockaddr *client_address, socklen_t *client_address_size);


// ================= SOCKET OPTIONS =================
void setTimeOutOpt(int socket, char *value, socklen_t value_len);

void setReuseAddrOpt(int socket, void *value, socklen_t value_len);

void setSocketNonblock(int socket);


// ================= SERVER INIT/DESTROY =================
int Init(int *locker_fd);

void Destroy(int server_fd);

void ender(int signal);


// ================= POST/GET =================
void buildResponse(char *envelope, char *message);

void sendResponse(int client, char *envelope);

bool getRequest(int client, char *buffer);


// ================= CLIENT =================
void *clientHandler(void *argc);

void addClient(pthread_t *cln);


// ================= IND TASKS =================
void getColor(char *envelope);

bool setColor(int code);

bool isChanged(char *current_value, char *snapshot);


// ================= PROCESS MONITORING =================
pthread_t InitMetricListener();

void *metricListener(void *argc);

void getPriority(char *buffer);


// ================================ Implementation ================================
// ===================================================================================================================
// ================= VARS =================
pthread_t monitor;
int priority;
bool changedMetric = false;

bool isAlive = false;
char *FONT_COLOR = {"\033[1;32;40m"};

pthread_t clients[MAX_CLIENT];

int logger, locker, server = -1;
int currentClientCount = 0;

// ================= HELPERS =================
void prettyPrint(char *msg, bool isError) {
    if (isError) {
        printf("%s[-]%s %s\n", ERROR_HEADER_COLOR, MESSAGE_COLOR, msg);
    } else {
        printf("%s[+]%s %s %s\n", INFO_HEADER_COLOR, GREEN_THEME_COLOR, msg, MESSAGE_COLOR);
    }
}

void errorHandler(char *msg) {
    setUnlock(locker);
    prettyPrint(msg, true);
    logging(logger, msg);

    exit(1);
}

void Info(char *msg) {
    logging(logger, msg);
    prettyPrint(msg, false);
}

void clearBuffer(char *buffer, int buffer_capacity) {
    memset(buffer, '\0', buffer_capacity);
}

void addCurDatetime(char *buff) {
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
    strcat(buff, datetime);
}


// ================= LOGGER =================
int getLogger() {
    int fd;
    if ((fd = open(LOGGER_FIFO_PATH, O_WRONLY)) == -1) {
        prettyPrint("Канал не открылся :(", true);
        perror("#");
    } else {
        prettyPrint("Логгер запушен", false);
    }

    return fd;
}

void logging(int logger_fd, char *msg) {
    if (logger_fd != -1) {
        char *record[1024];
        memset(record, '\0', 1024);
        strcat((char *) record, NAME);
        strcat((char *) record, msg);

        int i;
        if ((i = write(logger_fd, record, strlen(record))) != 0) {
            prettyPrint("Запись успешно отправлена логгеру", false);
        } else {
            prettyPrint("Запись не удалось отправить логгеру!", true);
        }
    }
}

void closeLogger(int logger_fd) {
    if (logger_fd != -1) {
        close(logger_fd);
        prettyPrint("Логгер закрыт", false);
    }
}


// ================= LOCK/UNLOCK =================
int setLock() {
    int fd;
    if ((fd = open(LOCKER_PATH, O_CREAT | O_RDWR, 0666)) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            errorHandler("Свервер уже запущен!");
        } else {
            errorHandler("Ошибка открытия файла блокировки!");
        }
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        errorHandler("Свервер уже запущен!");
    }

    Info("Блокировка на повторное создение экземпляра установлена");

    return fd;
}

void setUnlock(int locker_fd) {
    if (locker_fd != -1) {
        close(locker_fd);
        unlink(LOCKER_PATH);
    }

    Info("Блокировка убрана");
}


// ================= DECORATORS =================
int Socket(int domain, int type, int protocol) {
    int fd;
    if ((fd = socket(domain, type, protocol)) == -1) {
        errorHandler("Error creating socket!");
    }

    Info("TCP server socket created");

    return fd;
}

void Bind(int server_socket, struct sockaddr *server_address, int server_address_size) {
    if (bind(server_socket, server_address, server_address_size) < 0) {
        errorHandler("Bind error!");
    }

    Info("Bind complete");
}

void Listen(int server_socket, int capacity) {
    if (listen(server_socket, capacity) < 0) {
        errorHandler("Listen error");
    }

    Info("Listening...");
}

int Select(int fd_set_size, fd_set *fd_set_buffer_r, fd_set *fd_set_buffer_w, fd_set *fd_set_buffer_e,
           struct timeval *timeout_val) {
    int status;
    if ((status = select(fd_set_size, fd_set_buffer_r, fd_set_buffer_w, fd_set_buffer_e, timeout_val)) == -1) {
        errorHandler("Select error!");
    }

    return status;
}

int Accept(int server_socket, struct sockaddr *client_address, socklen_t *client_address_size) {
    int client_fd;
    return ((client_fd = accept(server_socket, (struct sockaddr *) client_address, client_address_size)) > 2)
           ? client_fd : -1;
}


// ================= SOCKET OPTIONS =================
void setTimeOutOpt(int socket, char *value, socklen_t value_len) {
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - RCVTIMEO");
    }

    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - SO_SNDTIMEO");
    }
}

void setReuseAddrOpt(int socket, void *value, socklen_t value_len) {
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, value, value_len) < 0) {
        errorHandler("SETSOCKOPT - REUSEADDR");
    }
}

void setSocketNonblock(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}


// ================= SERVER INIT/DESTROY =================
int Init(int *locker_fd) {
    *locker_fd = setLock();

    sleep(1);
    int sock = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    memset(&address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP);
    address.sin_port = htons(PORT);

    sleep(1);
    Bind(sock, (struct sockaddr *) &address, sizeof(address));
    sleep(1);
    Listen(sock, MAX_CLIENT);
    sleep(1);
    isAlive = true;

    monitor = InitMetricListener();
    sleep(1);
    Info("Сервер запущен");

    memset(clients, '\0', sizeof(clients));

    return sock;
}

void Destroy(int server_fd) {
    if (server_fd != -1) {
        close(server_fd);
        Info("Сервер завершил свою работу");
    }
}

void ender(int sig) {
    if (sig == SIGINT) {
        isAlive = false;
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (clients[i] != (void *) 0) {
                pthread_join(clients[i], NULL);
            }
        }

        Destroy(server);
        setUnlock(locker);
        closeLogger(logger);
    }

    exit(0);
}


// ================= POST/GET =================
void buildResponse(char *envelope, char *message) {
    clearBuffer(envelope, DEFAULT_CAPACITY);
    addCurDatetime(envelope);
    strcat(envelope, ": ");
    strcat(envelope, message);
}

void sendResponse(int client, char *envelope) {
    send(client, envelope, strlen(envelope), 0);
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
            clearBuffer(buffer, DEFAULT_CAPACITY);
            read(client, buffer, DEFAULT_CAPACITY);

            return true;
        }
    }

    return false;
}


// ================= CLIENT =================
void *clientHandler(void *argc) {
    int client = *(int *) argc;
    Info("Client connected");

    char *localValue[64];
    char *information = "\n[1] - Получить текущее значение цвета\n"
                        "[2] - Изменить цвет\n"
                        "[3] - Cпасательный круг\n"
                        "[4] - Ретироваться от сервера";
    char request[DEFAULT_CAPACITY];
    char response[DEFAULT_CAPACITY];
    char msg[512];

    buildResponse(response, "~~~~~ Hello ~~~~~");
    sendResponse(client, response);

    if (getRequest(client, request)) {
        buildResponse(response, information);
        sendResponse(client, response);
    }

    bool isInterrupted = false;
    while (isAlive) {
        if (changedMetric) {
            clearBuffer(msg, 512);
            strcat(msg, "Attention! Приоритет сервера '");
            strcat(msg, NAME);
            strcat(msg, "' изменился на: ");
            getPriority(msg);
            strcat(msg, "!");
            buildResponse(response, msg);
            sendResponse(client, response);
            changedMetric = false;
        }

        if (getRequest(client, request)) {
            clearBuffer(msg, 512);
            switch ((int) strtol(request, NULL, 10)) {
                case 1:
                    logging(logger, "Get color");
                    getColor(msg);

                    if (isChanged(msg, localValue)) {
                        buildResponse(response, msg);
                        sendResponse(client, response);
                    }
                    break;
                case 2:
                    logging(logger, "Set color");
                    buildResponse(response, "Выбери число от 0 до 6. Каждое число дифференцируется определенным оттенком");
                    sendResponse(client, response);

                    clearBuffer(request, DEFAULT_CAPACITY);
                    struct timeval timeout = {
                            .tv_sec = 20,
                            .tv_usec = 0,
                    };

                    fd_set set;
                    FD_ZERO(&set);
                    FD_SET(client, &set);
                    int status;
                    if ((status = Select(client + 1, &set, NULL, NULL, &timeout)) > 0) {
                        if (FD_ISSET(client, &set)) {
                            read(client, request, DEFAULT_CAPACITY);
                        }
                    } else if (status == 0) {
                        buildResponse(response, "Запрос сброшен: превышено время ожидания ответа!");
                        sendResponse(client, response);
                        continue;
                    }

                    int code = -1;
                    int index = strcspn(request, "\n");
                    if (request[index] == '\n') {
                        request[index] = '\0';
                    }

                    sscanf(request, "%d", &code);
                    buildResponse(response, setColor(code) ? "Успех!" : "Ошибка!");
                    sendResponse(client, response);

                    break;
                case 3:
                    logging(logger, "Help info");
                    buildResponse(response, information);
                    sendResponse(client, response);
                    break;
                case 4:
                    isInterrupted = true;
                    buildResponse(response, "514");
                    sendResponse(client, response);
                    break;
                default:
                    break;
            }
        }

        if (isInterrupted) {
            break;
        }
    }

    close(client);
    currentClientCount--;

    Info("Client disonnected");

    return NULL;
}

void addClient(pthread_t *cln) {
    clients[currentClientCount++] = *cln;
}


// ================= IND TASKS =================
void getColor(char *envelope) {
    strcat(envelope, FONT_COLOR);
    strcat(envelope, "\n");
    strcat(envelope, "\033[1;0;0m");
}

bool setColor(int code) {
    switch (code) {
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

bool isChanged(char *current_value, char *snapshot) {
    if (strcmp(current_value, snapshot) != 0) {
        clearBuffer(snapshot, 64);
        strcat(snapshot, current_value);

        return true;
    } else {
        return false;
    }
}


// ================= PROCESS MONITORING =================
pthread_t InitMetricListener() {
    pthread_t listener;
    if (pthread_create(&listener, NULL, metricListener, NULL) != 0) {
        errorHandler("Системный мониторинг не запустился :(");
    } else {
        Info("Системный мониторинг запустился");
    }

    return listener;
}

void *metricListener(void *argc) {
    int value;
    pid_t pid = getpid();

    while (isAlive) {
        value = getpriority(PRIO_PROCESS, (id_t) pid);
        if (priority != value) {
            priority = value;
            Info("Метрика процесса изменена");

            if (!changedMetric) {
                changedMetric = true;
            }
        }

        sleep(5);
    }

    return NULL;
}

void getPriority(char *buffer) {
    if (priority != NULL) {
        char *temp[5];

        clearBuffer(temp, 5);
        sprintf(temp, "%d", priority);
        strcat(buffer, temp);
    }
}


// ================= DRIVER =================
// ===================================================================================================================
int main() {
    signal(SIGINT, ender);

    logger = getLogger();
    server = Init(&locker);

    // Set up the socket configuration.
    struct timeval timeout = {
            .tv_sec = 4,
            .tv_usec = 0,
    };
    setTimeOutOpt(server, (char *) &timeout, sizeof(timeout));
    int optFlag = 1;
    setReuseAddrOpt(server, &optFlag, sizeof(optFlag));
//    setSocketNonblock(server);

    int flags = fcntl(server, F_GETFL, 0);
    fcntl(server, F_SETFL, flags | O_NONBLOCK);

    fd_set readableFds, actualFds;
    FD_ZERO(&readableFds);
    FD_SET(server, &readableFds);

    int client;
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize;

    int status;
    while (1) {
        actualFds = readableFds;
        status = Select(FD_SETSIZE, &actualFds, NULL, NULL, &timeout);
        if (status > 0) {
            for (int quessDesc = 0; quessDesc < FD_SETSIZE; quessDesc++) {
                if (FD_ISSET(quessDesc, &actualFds)) {
                    // If new connection.
                    if (server == quessDesc) {
                        if (currentClientCount < MAX_CLIENT) {
                            clientAddressSize = sizeof(clientAddress);
                            if ((client = Accept(server, (struct sockaddr *) &clientAddress,
                                                 &clientAddressSize)) != -1) {
                                int *newConnect = malloc(sizeof(int));
                                *newConnect = client;
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
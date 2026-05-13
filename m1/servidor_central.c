/*
 * servidor_central.c
 *
 * Servidor central IPv6 para arquitectura de mensajería instantánea.
 *
 * M1 escucha en TCP/3000.
 * El cliente envía:
 *
 *   GROUP 1
 *   GROUP 2
 *
 * Y el servidor responde con:
 *
 *   <IPv6_servidor_grupo> <puerto_tcp> <multicast_ipv6> <puerto_multicast>
 *
 * Compilación:
 *   gcc -Wall -Wextra -o servidor_central servidor_central.c
 *
 * Ejecución en M1:
 *   ./servidor_central
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 3000
#define BACKLOG 10
#define BUFFER_SIZE 256

#define GROUP_1_ID 1
#define GROUP_2_ID 2

#define GROUP_1_IPV6 "2001:720:1d10:fff0:216:3eff:fe97:d4e3"
#define GROUP_1_TCP_PORT 4001
#define GROUP_1_MULTICAST_IPV6 "ff15::1"
#define GROUP_1_MULTICAST_PORT 5001

#define GROUP_2_IPV6 "2001:720:1d10:fff0:216:3eff:fe4f:b1a8"
#define GROUP_2_TCP_PORT 4002
#define GROUP_2_MULTICAST_IPV6 "ff15::2"
#define GROUP_2_MULTICAST_PORT 5002

static volatile sig_atomic_t running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

static void remove_newline(char *str)
{
    if (str == NULL) {
        return;
    }

    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static void build_group_response(int group_id, char *response, size_t response_size)
{
    if (response == NULL || response_size == 0) {
        return;
    }

    if (group_id == GROUP_1_ID) {
        snprintf(response, response_size,
                 "%s %d %s %d\n",
                 GROUP_1_IPV6,
                 GROUP_1_TCP_PORT,
                 GROUP_1_MULTICAST_IPV6,
                 GROUP_1_MULTICAST_PORT);
    } else if (group_id == GROUP_2_ID) {
        snprintf(response, response_size,
                 "%s %d %s %d\n",
                 GROUP_2_IPV6,
                 GROUP_2_TCP_PORT,
                 GROUP_2_MULTICAST_IPV6,
                 GROUP_2_MULTICAST_PORT);
    } else {
        snprintf(response, response_size,
                 "ERROR grupo_no_existente\n");
    }
}

static void handle_client(int client_fd, const struct sockaddr_in6 *client_addr)
{
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    char client_ip[INET6_ADDRSTRLEN];
    int client_port;

    memset(buffer, 0, sizeof(buffer));
    memset(response, 0, sizeof(response));
    memset(client_ip, 0, sizeof(client_ip));

    inet_ntop(AF_INET6,
              &(client_addr->sin6_addr),
              client_ip,
              sizeof(client_ip));

    client_port = ntohs(client_addr->sin6_port);

    printf("[M1] Cliente conectado desde [%s]:%d\n", client_ip, client_port);

    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read < 0) {
        perror("[M1] Error en recv");
        close(client_fd);
        return;
    }

    if (bytes_read == 0) {
        printf("[M1] Cliente cerró la conexión sin enviar datos\n");
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    remove_newline(buffer);

    printf("[M1] Petición recibida: '%s'\n", buffer);

    int group_id = -1;

    if (sscanf(buffer, "GROUP %d", &group_id) == 1) {
        build_group_response(group_id, response, sizeof(response));
    } else {
        snprintf(response, sizeof(response),
                 "ERROR formato_invalido usar: GROUP <id>\n");
    }

    ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);

    if (bytes_sent < 0) {
        perror("[M1] Error en send");
    } else {
        printf("[M1] Respuesta enviada: %s", response);
    }

    close(client_fd);
    printf("[M1] Conexión cerrada\n\n");
}

int main(void)
{
    int server_fd;
    int optval = 1;

    struct sockaddr_in6 server_addr;

    signal(SIGINT, handle_sigint);

    server_fd = socket(AF_INET6, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("[M1] Error creando socket IPv6");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) < 0) {
        perror("[M1] Error en setsockopt SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(SERVER_PORT);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("[M1] Error en bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("[M1] Error en listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("=============================================\n");
    printf(" Servidor central M1 iniciado\n");
    printf(" Escuchando en IPv6 TCP puerto %d\n", SERVER_PORT);
    printf("=============================================\n");
    printf("Tabla de grupos:\n");
    printf(" Grupo 1 -> [%s]:%d multicast %s:%d\n",
           GROUP_1_IPV6,
           GROUP_1_TCP_PORT,
           GROUP_1_MULTICAST_IPV6,
           GROUP_1_MULTICAST_PORT);
    printf(" Grupo 2 -> [%s]:%d multicast %s:%d\n",
           GROUP_2_IPV6,
           GROUP_2_TCP_PORT,
           GROUP_2_MULTICAST_IPV6,
           GROUP_2_MULTICAST_PORT);
    printf("=============================================\n\n");

    while (running) {
        struct sockaddr_in6 client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);

        if (client_fd < 0) {
            if (errno == EINTR && !running) {
                break;
            }

            perror("[M1] Error en accept");
            continue;
        }

        handle_client(client_fd, &client_addr);
    }

    printf("\n[M1] Cerrando servidor central...\n");
    close(server_fd);

    return 0;
}

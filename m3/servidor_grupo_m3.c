/*
 * servidor_grupo_m3.c
 *
 * Servidor de grupo 2 para la arquitectura de mensajería instantánea IPv6.
 *
 * Este servidor se ejecuta en M3.
 *
 * Funciones:
 *   - Escucha mensajes TCP en el puerto 4002.
 *   - Recibe mensajes enviados por los clientes.
 *   - Guarda los mensajes en el fichero grupo_2.log.
 *   - Reenvía los mensajes por UDP multicast IPv6 a ff15::2:5002.
 *
 * Compilación:
 *   gcc -Wall -Wextra -o servidor_grupo_m3 servidor_grupo_m3.c
 *
 * Ejecución en M3:
 *   ./servidor_grupo_m3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define GROUP_ID 2

#define TCP_PORT 4002

#define MULTICAST_IPV6 "ff15::2"
#define MULTICAST_PORT 5002

#define LOG_FILE "grupo_2.log"

#define BACKLOG 10
#define BUFFER_SIZE 512
#define MESSAGE_SIZE 1024

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

static void get_timestamp(char *buffer, size_t size)
{
    time_t now;
    struct tm *tm_info;

    if (buffer == NULL || size == 0) {
        return;
    }

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info == NULL) {
        snprintf(buffer, size, "sin_fecha");
        return;
    }

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static int save_message_to_file(const char *message)
{
    FILE *file;

    if (message == NULL) {
        return -1;
    }

    file = fopen(LOG_FILE, "a");

    if (file == NULL) {
        perror("[M3] Error abriendo fichero grupo_2.log");
        return -1;
    }

    fprintf(file, "%s", message);

    if (fflush(file) != 0) {
        perror("[M3] Error haciendo fflush del histórico");
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        perror("[M3] Error cerrando fichero del histórico");
        return -1;
    }

    return 0;
}

static int create_tcp_server_socket(void)
{
    int server_fd;
    int optval = 1;
    struct sockaddr_in6 server_addr;

    server_fd = socket(AF_INET6, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("[M3] Error creando socket TCP IPv6");
        return -1;
    }

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &optval,
                   sizeof(optval)) < 0) {
        perror("[M3] Error en setsockopt SO_REUSEADDR");
        close(server_fd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons((uint16_t)TCP_PORT);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("[M3] Error en bind TCP");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("[M3] Error en listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

static int create_multicast_socket(void)
{
    int udp_fd;
    int hops = 16;

    udp_fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (udp_fd < 0) {
        perror("[M3] Error creando socket UDP IPv6");
        return -1;
    }

    /*
     * Número máximo de saltos multicast.
     * Para la práctica local bastaría con 1, pero dejamos 16 por seguridad.
     */
    if (setsockopt(udp_fd,
                   IPPROTO_IPV6,
                   IPV6_MULTICAST_HOPS,
                   &hops,
                   sizeof(hops)) < 0) {
        perror("[M3] Aviso: no se pudo configurar IPV6_MULTICAST_HOPS");
    }

    return udp_fd;
}

static int send_multicast_message(int udp_fd, const char *message)
{
    struct sockaddr_in6 multicast_addr;

    if (message == NULL) {
        return -1;
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));

    multicast_addr.sin6_family = AF_INET6;
    multicast_addr.sin6_port = htons((uint16_t)MULTICAST_PORT);

    if (inet_pton(AF_INET6,
                  MULTICAST_IPV6,
                  &multicast_addr.sin6_addr) != 1) {
        fprintf(stderr,
                "[M3] Dirección multicast IPv6 inválida: %s\n",
                MULTICAST_IPV6);
        return -1;
    }

    ssize_t sent = sendto(udp_fd,
                          message,
                          strlen(message),
                          0,
                          (struct sockaddr *)&multicast_addr,
                          sizeof(multicast_addr));

    if (sent < 0) {
        perror("[M3] Error enviando por multicast");
        return -1;
    }

    return 0;
}

static void handle_client(int client_fd,
                          const struct sockaddr_in6 *client_addr,
                          int udp_fd)
{
    char buffer[BUFFER_SIZE];
    char final_message[MESSAGE_SIZE];
    char timestamp[64];

    char client_ip[INET6_ADDRSTRLEN];
    int client_port;

    memset(buffer, 0, sizeof(buffer));
    memset(final_message, 0, sizeof(final_message));
    memset(timestamp, 0, sizeof(timestamp));
    memset(client_ip, 0, sizeof(client_ip));

    inet_ntop(AF_INET6,
              &(client_addr->sin6_addr),
              client_ip,
              sizeof(client_ip));

    client_port = ntohs(client_addr->sin6_port);

    printf("[M3] Cliente conectado desde [%s]:%d\n",
           client_ip,
           client_port);

    ssize_t bytes_read = recv(client_fd,
                              buffer,
                              sizeof(buffer) - 1,
                              0);

    if (bytes_read < 0) {
        perror("[M3] Error en recv");
        close(client_fd);
        return;
    }

    if (bytes_read == 0) {
        printf("[M3] Cliente cerró la conexión sin enviar datos\n");
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    remove_newline(buffer);

    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(final_message,
             sizeof(final_message),
             "[%s] Grupo %d | origen [%s]:%d | mensaje: %s\n",
             timestamp,
             GROUP_ID,
             client_ip,
             client_port,
             buffer);

    printf("[M3] Mensaje recibido por TCP:\n%s",
           final_message);

    if (save_message_to_file(final_message) == 0) {
        printf("[M3] Mensaje guardado en %s\n", LOG_FILE);
    } else {
        printf("[M3] No se pudo guardar el mensaje en %s\n", LOG_FILE);
    }

    if (send_multicast_message(udp_fd, final_message) == 0) {
        printf("[M3] Mensaje reenviado por multicast a [%s]:%d\n",
               MULTICAST_IPV6,
               MULTICAST_PORT);
    } else {
        printf("[M3] No se pudo reenviar el mensaje por multicast\n");
    }

    const char *ack = "OK mensaje_recibido\n";

    if (send(client_fd, ack, strlen(ack), 0) < 0) {
        perror("[M3] Error enviando ACK");
    }

    close(client_fd);

    printf("[M3] Conexión cerrada\n\n");
}

int main(void)
{
    int server_fd;
    int udp_fd;

    signal(SIGINT, handle_sigint);

    server_fd = create_tcp_server_socket();

    if (server_fd < 0) {
        exit(EXIT_FAILURE);
    }

    udp_fd = create_multicast_socket();

    if (udp_fd < 0) {
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("=============================================\n");
    printf(" Servidor M3 - Grupo 2 iniciado\n");
    printf(" Escuchando TCP IPv6 en puerto: %d\n", TCP_PORT);
    printf(" Multicast IPv6: [%s]:%d\n", MULTICAST_IPV6, MULTICAST_PORT);
    printf(" Histórico: %s\n", LOG_FILE);
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

            perror("[M3] Error en accept");
            continue;
        }

        handle_client(client_fd, &client_addr, udp_fd);
    }

    printf("\n[M3] Cerrando servidor de grupo 2...\n");

    close(udp_fd);
    close(server_fd);

    return 0;
}
